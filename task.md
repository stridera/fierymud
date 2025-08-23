# Next Steps

- Investigate an issue that occurs when a command is sent after logging in with "tester:tester" using the prod config.  I can see the command runs in the server logs, but nothing is shown in telnet.
- Figure out the config system.  Unify the two.  Fix things like players not being saved correctly to the player_directory in the config.
- If a character ends up in 'nowhere', send them to the starting room.  If the starting room does not exist, send them to the first room you can find.
- Review and ensure all existing tests are running correctly.
- Explore opportunities to improve the overall test suite.
