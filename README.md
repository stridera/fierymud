# fierymud

The original vision of FieryMUD was to create a challenging MUD for advanced players. This Fiery is reborn in the hope of bringing back the goals of the past by inflicting certain death on unsuspecting players...  
FieryMUD will continue to grow and change through the coming years and those players who seek challange and possess imagination will come in search of what the 3D world fails to offer them.

You can play at telnet://fierymud.org:4000

## Configuration:
The first time you run the mud, you'll need to copy the lib.default to lib.  This has the following folder structure:
- etc: Contains board, clan, and mail data
- misc: Stores bugs, ideas, quests, socials, typos, and xnames (banned names).  All text files.
- text: Stores help, motds, and other text for showing players in game.
- world: Stores the areas, items, mobs, etc.

## Building:
Inside the root directory, building should be pretty simple
```
cmake -B build --install-prefix /opt/MUD/ .
cmake --build build --target install
```
Replace /opt/MUD/ with the location of where you want to build it.

If you're using vscode, you can debug it with the following configuration:
```
        {
            "type": "lldb",
            "request": "launch",
            "name": "Debug",
            "program": "${workspaceFolder}/build/fierymud",
            "args": [],
            "cwd": "${workspaceFolder}"
        }
```
