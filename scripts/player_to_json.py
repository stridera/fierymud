import argparse
import json
import os

from mud import Encoder, MudFiles


def main(path: str, player: str, output: str = None):
    for player_files in MudFiles.player_files(path, player):
        current_player = player_files.id
        print(f"Processing {current_player}.")
        player = {}
        for file in player_files.files:
            print(f"-- Processing {file.get_mud_type()} for player {current_player} ({file.filename})")
            player[file.mud_type.name] = file.parse_player()

        if output is None:
            out_file = os.path.join(player_files.path, f"{current_player}.json")
        elif output == "stdout":
            out_file = "stdout"
        elif output.endswith(".json"):
            out_file = output
        else:
            out_file = os.path.join(output, f"{current_player}.json")

        if out_file == "stdout":
            print(json.dumps(player, cls=Encoder, indent=4))
        else:
            print(f"-- Writing {out_file} ...")
            file = open(out_file, "w", encoding="ascii")
            file.write(json.dumps(player, cls=Encoder, indent=4))
            file.write("\n")

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
