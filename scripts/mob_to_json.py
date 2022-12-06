import argparse
import os
from mud_objects import Mob


def process_file(filename, vnum=-1):
    print(f"Processing {filename}")
    current_vnum = -1
    mobs = []
    data = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.rstrip()
            if line.startswith('#'):
                if current_vnum >= 0 and (vnum == -1 or vnum == current_vnum):
                    print(f"Processing vnum {current_vnum}")
                    mob = Mob(current_vnum)
                    # print(f"Data: {data}\n\n")
                    mob.parse(data)
                    mobs.append(mob)
                current_vnum = int(line[1:])
                data = []
            else:
                data.append(line)

    with open(os.path.splitext(filename)[0]+'.json', 'w') as f:
        for mob in mobs:
            f.write(mob.to_json())
            f.write('\n')


def process_index(index_file, vnum):
    path = index_file[:-5]
    with open(index_file) as f:
        for line in f:
            if line.startswith('$'):
                print('End of index')
                return
            elif line.rstrip().endswith('.mob'):
                process_file(path + line.rstrip())


def main(mob_file, vnum):
    if mob_file.endswith('index'):
        process_index(mob_file, vnum)
    else:
        process_file(mob_file, vnum)


if __name__ == '__main__':
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Converts a mob file to use ascii flags.')
    parser.add_argument('--file', help='The mob file to convert or index file to convert all.',
                        type=str, default='../lib/world/mob/index')
    parser.add_argument('-n', '--number', help='The mob number to convert.', type=int, default=-1)
    args = parser.parse_args()
    main(args.file, args.number)
