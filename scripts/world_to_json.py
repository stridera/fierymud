
import argparse
import json
import os

from mud import Encoder, MudFiles
from mud.validator_visitor import validate_all


def main(path: str, zone: int | None, output: str | None = None):
    processed_files = 0
    all_zone_data = []
    for zone_files in MudFiles.zone_files(path, zone):
        zone_id = zone_files.id
        print(f"\nProcessing Zone {zone_id}...")
        prototype = {}
        for file in zone_files.files:
            print(f"-- Processing {file.get_mud_type()} file: {file.filename}")
            val = file.parse_world()
            # Always store as a list: None -> [], single object -> [obj], list -> list
            if val is None:
                prototype[file.get_json_id()] = []
            elif isinstance(val, list):
                prototype[file.get_json_id()] = val
            else:
                prototype[file.get_json_id()] = [val]
        prototype["zone_id"] = zone_id
        prototype["zone_path"] = zone_files.path
        all_zone_data.append(prototype)

    # Merge all zones for global validation
    merged = {}
    for proto in all_zone_data:
        for k, v in proto.items():
            if k in ("zone_id", "zone_path"): continue
            if k not in merged:
                merged[k] = []
            merged[k].extend(v)

    # Centralized validation step (after all zones loaded)
    print("-- Validating all zones (global references)...")
    error_log = validate_all(merged)
    if error_log:
        log_path = os.path.join(path, "all_zones_validation_errors.log")
        print(f"-- Validation errors found. Writing to {log_path}")
        with open(log_path, "w", encoding="utf-8") as logf:
            for err in error_log:
                logf.write(err + "\n")

    # Apply validation results back to original zone data by filtering out invalid items
    # Create sets of valid IDs from the validated merged data using the same logic as validator
    def get_id_from_obj(obj):
        if hasattr(obj, 'id'):
            return obj.id
        if isinstance(obj, dict) and 'id' in obj:
            return obj['id']
        if hasattr(obj, 'vnum'):
            return obj.vnum
        if isinstance(obj, dict) and 'vnum' in obj:
            return obj['vnum']
        return None

    valid_shop_ids = set(str(get_id_from_obj(shop)) for shop in merged.get('shops', []) if get_id_from_obj(shop) is not None)
    valid_object_ids = set(str(get_id_from_obj(obj)) for obj in merged.get('objects', []) if get_id_from_obj(obj) is not None)
    valid_mob_ids = set(str(get_id_from_obj(mob)) for mob in merged.get('mobs', []) if get_id_from_obj(mob) is not None)
    valid_room_ids = set(str(get_id_from_obj(room)) for room in merged.get('rooms', []) if get_id_from_obj(room) is not None)

    # Filter each zone's data to only include valid items
    for proto in all_zone_data:
        # Filter shops
        if 'shops' in proto:
            proto['shops'] = [shop for shop in proto['shops']
                            if str(get_id_from_obj(shop)) in valid_shop_ids]
        # Filter objects
        if 'objects' in proto:
            proto['objects'] = [obj for obj in proto['objects']
                              if str(get_id_from_obj(obj)) in valid_object_ids]
        # Filter mobs
        if 'mobs' in proto:
            proto['mobs'] = [mob for mob in proto['mobs']
                           if str(get_id_from_obj(mob)) in valid_mob_ids]
        # Filter rooms
        if 'rooms' in proto:
            proto['rooms'] = [room for room in proto['rooms']
                            if str(get_id_from_obj(room)) in valid_room_ids]

    # Write output per zone (now validated)
    for proto in all_zone_data:
        zone_id = proto["zone_id"]
        zone_files_path = proto["zone_path"]
        # Remove helper keys
        out_proto = {k: v for k, v in proto.items() if k not in ("zone_id", "zone_path")}
        # Unwrap zone object if it's a single-item list
        for k in list(out_proto.keys()):
            if k in ("zone", "zones") and isinstance(out_proto[k], list) and len(out_proto[k]) == 1:
                out_proto[k] = out_proto[k][0]
        if output is None:
            out_file = os.path.join(zone_files_path, f"{zone_id}.json")
        elif output == "stdout":
            out_file = "stdout"
        elif output.endswith(".json"):
            out_file = output
        else:
            out_file = os.path.join(output, f"{zone_id}.json")

        if out_file == "stdout":
            print(json.dumps(out_proto, cls=Encoder, indent=4))
        else:
            print(f"-- Writing {out_file} ...")
            with open(out_file, "w", encoding="ascii") as file:
                file.write(json.dumps(out_proto, cls=Encoder, indent=4))
                file.write("\n")
            processed_files += 1

        print(f"Done processing Zone {zone_id}")
    print(f"Total files processed: {processed_files}")


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
