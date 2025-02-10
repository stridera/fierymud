# Python Scripts
These scripts are used to convert Fierymud mud files to json files.

## Scripts

### mudfile_to_json
This script processes the world files (lib/world/*) and converts them to a json file in the same folder as the source file.

Commandline Parameters
file: Required positional.  Must be a mud file (.mob, .zon, etc) or an index.
--type <type> - Force the file type.  Can be used if you're parsing a file without a proper file extension.  Should be automatically guessed.  Must be one of: mob, obj, zon, shp, wld.
--output <path> - Specify the file output location.  Defaults to same directory as source file.




## Setup
```bash
cd scripts
poetry 

## Examples
### Update  World Files (/lib/)
Single File:
```bash
poetry run python mudfile_to_json.py ../lib/world/wld/30.wld
```

Run against the whole codebase
```bash
for f in $(find ../lib/world -name index); do echo $f; poetry run python mudfile_to_json.py $f; done```

Note:  You can change the `../lib` to any directory if you want to only run it against one folder (like mob or zon for example.)


### Player Files
View Players objects:
```bash
poetry run python player_objects.py --player strider --list
```

Create player json files for all players
```bash
poetry run python player_objects.py
```
