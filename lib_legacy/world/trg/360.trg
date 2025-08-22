#36001
Kaaz_semantic_fight1~
0 k 14
~
say You've got nothing for me!
mosh %actor.name%
~
#36002
Kaaz_semantic_fight2~
0 k 16
~
say So you think you are mighty enough to take me?!
laugh %actor.name%
~
#36003
Large_seether_death~
0 f 100
~
* Does next mob in spawn cycle already exist?
if %get.mob_count[8031]% < 1 && (%actor.level% < 30 || %actor.quest_stage[dragon_slayer]% == 3)
  m_run_room_trig 36006
  * Generate random room number to spawn drakes in.
  * Thanks to the evil Pergus for inspiring me to be
  * more evil.
  set rnd_range %random.126%
  eval rnd_room %rnd_range% + 8049
  mat 16095 mload mob 8031
  set rnd %random.100%
  mat 16095 mteleport ice %rnd_room%
  * Sometimes creatures don't get teleported out of the loading
  * room so we're gonna go back and purge it just in case.
  mat 16095 mpurge
endif
~
#36004
wug_quest_seether_greet~
0 g 100
~
if %actor.vnum% == -1
  if %get.mob_count[8031]% < 1 && %actor.level% < 30
    wait 2
    hiss %actor%
    say You can't have it!  You'll just break it!
    mecho %self.name% clutches a crystalline amulet to its chest!
  endif
endif
~
#36005
genasi-sigh~
0 k 20
~
   sigh
   say I don't really have time for this right now.
c 'teleport'
* Should only teleport to 23832-23869, other rooms are outside its range
~
#36006
wug_quest_load_draklings~
2 a 100
~
wait 2
wecho %get.mob_shortdesc[36004]% lets slip a crystalline amulet which shatters on impact!
wecho %get.mob_shortdesc[36004]% groans, 'They will now be free of our prison...'
wait 1s
wecho All the seethers begin chittering!
wait 1s
wecho Off in the distance you hear the ROAR of a dragon of some kind!
wecho It seems to be coming from the farmlands to the south of the temple.
~
#36010
Test_master_control_trigger~
0 n 100
mama~
m_run_room_trig 36011
say debug1
~
#36011
test2~
2 cfg 0
~
wforce girbina say Yer mama
wecho debug2
~
#36020
Girbina_Tickmaster_greet~
0 g 100
~
*
* Greet trigger got Girbina Tickmaster
*
if (%actor.vnum% != -1 || %actor.level% > 99)
    halt
endif
if (%actor.class% == sorcerer)
wait 2
   * De-sluttify DG variable
   unset port_quest
   *
   * Conditionals for which quest is being done.  Note we're going
   * to require these be done in order.
   *
   if %actor.level% >= 25 && !%actor.has_completed[speculan_wilderland]%
      set port_quest SW
      say you will be on SW
   endif
   if %actor.level% >= 33 && (%actor.has_completed[speculan_wilderland]% && !%actor.has_completed[nikozian_ice]%)
      set port_quest NI
      say you will be on NI
   endif
   if %actor.level% >= 41 && (%actor.has_completed[nikozian_ice]% && !%actor.has_completed[stonewardens_promise]%)
      say you will be on SW
      set port_quest SW
   endif
   if %actor.level% >= 41 && (%actor.has_completed[stonewardens_promise]% && !%actor.has_completed[parcel_to_sw]%)
      set port_quest PtSW
      say you will be on PtSW
   endif
   if %actor.level% >= 49 && (%actor.has_completed[parcel_to_sw]% && !%actor.has_completed[winds_of_gothra]%)
      say you will be on WoG
      set port_quest WoG
   endif
   if %actor.level% >= 49 && (%actor.has_completed[winds_of_gothra]% && !%actor.has_completed[ruck_to_ni]% )
      set port_quest RtNI
      say you will be on RtNI
   endif
   if %actor.level% >= 57 && (%actor.has_completed[ruck_to_ni]% && !%actor.has_completed[muster_of_sp]% )
      say you will be on MoSP
      set port_quest MoSP
   endif
   if %actor.level% >= 65 && (%actor.has_completed[muster_of_sp]% && !%actor.has_completed[crush_the_wog]% )
      set port_quest CtWoG
      say you will be on CtWoG
   endif
   if %actor.level% >= 73 && (%actor.has_completed[crush_the_wog]% && !%actor.has_completed[eldoria_proper]% )
      say you will be on EP
      set port_quest EP
   endif
   if %actor.level% >= 81 && (%actor.has_completed[eldoria_proper]% && !%actor.has_completed[assembly_of_ep]% )
      set port_quest AoEP
      say you will be on AoEP
   endif
   * Say IT
   if %port_quest% == SW
      say You are on SW
   endif
   if %port_quest% == NI
      say You are on NI
   endif
   if %port_quest% == SP
      say You are on SP
   endif
   if %port_quest% == PtSW
      say You are on PtSW
   endif
   if %port_quest% == WoG
      say You are on WoG
   endif
   if %port_quest% == RtNI
      say You are on RtNI
   endif
   if %port_quest% == MoSP
      say You are on MoSP
   endif
   if %port_quest% == CtWoG
      say You are on SW
   endif
   if %port_quest% == EP
      say You are on EP
   endif
   if %port_quest% == AoEP
      say You are on AoEP
   endif
else
   halt
endif
~
$~
