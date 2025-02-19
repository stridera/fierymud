import argparse
import json
import os

from mud import Encoder, MudFiles


def main(path: str, zone: int | None, output: str = None):
    for zone_files in MudFiles.zone_files(path, zone):
        zone_id = zone_files.id
        print(f"\nProcessing Zone {zone_id}...")
        prototype = {}
        for file in zone_files.files:
            print(f"-- Processing {file.get_mud_type()} file: {file.filename}")
            prototype[file.mud_type.name] = file.parse_world()

        if output is None:
            out_file = os.path.join(zone_files.path, f"{zone_id}.json")
        elif output == "stdout":
            out_file = "stdout"
        elif output.endswith(".json"):
            out_file = output
        else:
            out_file = os.path.join(output, f"{zone_id}.json")

        if out_file == "stdout":
            print(json.dumps(prototype, cls=Encoder, indent=4))
        else:
            print(f"-- Writing {out_file} ...")
            file = open(out_file, "w", encoding="ascii")
            file.write(json.dumps(prototype, cls=Encoder, indent=4))
            file.write("\n")

        print("Done")


if __name__ == "__main__":
    # Parse command line arguments and get the filename to process
    parser = argparse.ArgumentParser(
        description="Convert a mudfile file to JSON.",
        epilog="If the file is an index file, all files in the root index will be converted.",
    )
    parser.add_argument(
        "--path",
        help="The path to the mudfile index file.  Example: ../lib/world/",
        type=str,
        default="../lib/world/",
    )
    parser.add_argument(
        "--zone",
        help="The file to convert or index file to convert all.  Example: 30",
        type=int,
    )
    parser.add_argument(
        "--output",
        help="The output file to write to.  If not specified, the output file will be the same as the input file with the extension changed to .json",
        type=str,
    )
    args = parser.parse_args()
    main(args.path, args.zone, args.output)
