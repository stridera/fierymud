import argparse
import os
import json
from mud import Encoder, MudFile
from mud import Mob, Obj, Zone, Shop, World


def main(filename: str, type: str, output: str = None):
    cls = None
    if type is None:
        # Try to guess the type from the filename
        if filename.split('/')[-2] in ['mob', 'obj', 'zon', 'shp', 'wld']:
            type = filename.split('/')[-2]
            print(f"Guessing type: {type}")

    if type == "mob":
        cls = Mob
    elif type == "obj":
        cls = Obj
    elif type == "zon":
        cls = Zone
    elif type == "shp":
        cls = Shop
    elif type == "wld":
        cls = World
    else:
        print(f"Unknown type: {type}")
        return

    output_file = None
    if output is not None:
        print(f"Writing to {output}")
        output_file = open(output, "w", encoding="ascii")

    for mudfile in MudFile.from_filename(filename):
        print(f"Processing {mudfile.filename}...", end=" ")
        protos = cls.from_mudfile(mudfile)

        # for proto in protos:
        #     output_file.write(proto.to_json() + "\n")

        if output is None:
            json_file = os.path.splitext(mudfile.current_file())[0] + ".json"
            if output_file is not None:
                output_file.close()
            output_file = open(json_file, "w", encoding="ascii")
            print(f"Writing {json_file}...", end=" ")

        output_file.write(json.dumps(protos, cls=Encoder))

        print("Done")


if __name__ == "__main__":
    # Parse command line arguments and get the filename to process
    parser = argparse.ArgumentParser(
        description="Convert a mudfile file to JSON.",
        epilog="If the file is an index file, all files in the index will be converted.",
    )
    # Get positional filename argument
    parser.add_argument(
        "file",
        help="The file to convert or index file to convert all.  Example: ../lib/world/mob/index",
        type=str,
    )
    parser.add_argument(
        "--type",
        help="The type of file to convert.  Example: mob, obj, zone, shp, wld, zon",
        type=str,
    )
    parser.add_argument(
        "--output",
        help="The output file to write to.  If not specified, the output file will be the same as the input file with the extension changed to .json",
        type=str,
    )
    args = parser.parse_args()
    main(args.file, args.type, args.output)
