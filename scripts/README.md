# Python Scripts
These scripts are used to convert Fierymud mud files to json files.

## Scripts

### World File (mudfile_to_json)
This script processes the world files (lib/world/*) and converts them to a json file in the same folder as the source file.

Use the `--help` command line argument to view all arguments.

## Setup
```bash
cd scripts
poetry 

## Examples
### Update  World Files (/lib/)
Single File:
```bash
poetry run python mudfile_to_json.py --zone 30
```

Run against the whole codebase
```bash
poetry run python mudfile_to_json.py
```

Place output in another location
```bash
poetry run python mudfile_to_json.py --output ./zones
```


### Player Files (player_to_json)
Create player json files for all players:
```bash
poetry run python player_to_json.py
```

Create single player json and print to stdout
```bash
poetry run python player_to_json.py --player strider --output stdout
```
