import argparse
import os
import json
from mud import Encoder, MudFile
from mud import Player, PlayerObj, PlayerPets, PlayerQuests, PlayerNotes


def main(path: str, player: str, output: str = None):
    for player_file in MudFile.player_files(path, player):
        print(f"Processing {player_file['name']}.")
        player = {}
        for file in player_file["files"]:
            print(f"-- Processing {file['class']} - {file['filename']}")
            cls = None
            match file["class"]:
                case "Player":
                    cls = Player
                case "Objects":
                    cls = PlayerObj
                case "Pets":
                    cls = PlayerPets
                case "Quests":
                    cls = PlayerQuests
                case "Notes":
                    cls = PlayerNotes
                case _:
                    print(f"Unknown class: {file['class']}")
                    continue
            mudfile = MudFile(file["filename"])
            player[file["class"]] = cls.from_mudfile(mudfile)

        if args.output is None:
            output_file = os.path.splitext(mudfile.filename)[0] + ".json"
        else:
            output_file = args.output
        with open(output_file, "w", encoding="ascii") as f:
            f.write(json.dumps(player, cls=Encoder, indent=4))
        print(f"----  Writing {output_file}")

    print("Done")


if __name__ == "__main__":
    # Parse command line arguments and get the filename to process
    parser = argparse.ArgumentParser(
        description="Convert player files to JSON.",
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
        "--output",
        help="The output file to write to.  If not specified, the output file will be the same as the input file with the extension changed to .json",
        type=str,
    )
    args = parser.parse_args()
    main(args.path, args.player, args.output)
