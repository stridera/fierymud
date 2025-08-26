# FieryMUD Modernization Task Status

We have done a lot, but there is a lot of stuff marked as 'complete' that aren't fully functional yet.  Start looking at our tests and make sure they are comprehensive.  We should probably clean up the tests folder as well to make sure it's easy to identify unit/integration tests and framework/fixtures.

**Recommended Next Steps**: 
- The entity start room is not being set.  When they die, they can't revive because their start room is 0.  There appears to be a large discrepency between entity and their derived classes.  
- Let's add general information tasks to the whitelist of commands that can be run while dead.  For example, ghosts should still be able to run 'help', 'score', 'inventory', 'equipment', 'commands', etc.
- Can we add a prompt that shows after every command.  For now it can be basic, but it should show your health and movement, and the enemies condition if fighting.
- Task J (Object Container and Interaction System) - builds on the completed combat system to enhance object interaction, equipment, containers, and world object mechanics. This will provide a more immersive player experience and complete the core gameplay loop.
- Update this file with a concise plan for our next steps for the next iteration.