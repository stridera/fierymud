import argparse
import os

from mud import MudFile
from mud import PlayerObj


def list_items(items, indent=0):
    for item in items:
        stats = item.stats
        print(f" {' [---->]' * indent} {stats['location']} [{item.vnum:5}] {item.decolor(stats['short_desc'])}")
        if item.contents:
            list_items(item.contents, indent + 1)


def main(args):
    if args.player is None and args.list:
        print("You must specify a player if you want to list their inventory.")
        return

    for plrfiles in MudFile.player_files(args.path, args.player):
        print(f"Processing {plrfiles['name']}...", end=" ")
        if "objs" not in plrfiles:
            print("No objects file found.")
            continue
        mudfile = MudFile(plrfiles["objs"])
        items = PlayerObj.from_mudfile(mudfile)
        if args.list:
            list_items(items)


if __name__ == "__main__":
    # Parse command line arguments and get the filename to process
    parser = argparse.ArgumentParser(
        description="Convert a mudfile file to JSON.",
        epilog="If the file is an index file, all files in the index will be converted.",
    )
    parser.add_argument(
        "--path",
        help="The path to the player files.  If not specified, the default is ../lib/players",
        type=str,
        default="../lib/players",
    )
    parser.add_argument(
        "--player",
        help="The player.  If not specified, all players will be selected.",
        type=str,
    )
    parser.add_argument(
        "--verify",
        help="Verify the players items by testing against the verify item list.  You can generate this file by doing `python ./mudfile_to_json.py ../lib/world/obj/index --output items.json`",
        type=str,
    )
    parser.add_argument(
        "--list",
        help="List the players items.",
        action="store_true",
    )
    parser.add_argument(
        "--output",
        help="The output file to write to.  If not specified, the output file will be the same as the input file with the extension changed to .json",
        type=str,
    )
    args = parser.parse_args()
    main(args)
