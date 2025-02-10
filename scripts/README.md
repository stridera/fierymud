# Python Scripts
These scripts are used to convert Fierymud mud files to json files.

## Setup
```bash
cd scripts
poetry 

## Examples
Single File:
```bash
poetry run python mudfile_to_json.py ../lib/world/wld/30.wld
```

Run against the whole codebase
```bash
for f in $(find ../lib -name index); do echo $f; poetry run python mudfile_to_json.py $f; done```

Note:  You can change the `../lib` to any directory if you want to only run it against one folder (like mob or zon for example.)