import argparse
from os import rename
from mud import Mob


def process_file(filename, id=-1):
    print(f"Processing {filename}")
    current_id = -1
    mobs = []
    data = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.rstrip()
            if line.startswith('#'):
                if current_id >= 0 and (id == -1 or id == current_id):
                    print(f"Processing id {current_id}")
                    mob = Mob(current_id)
                    # print(f"Data: {data}\n\n")
                    mob.parse(data)
                    mobs.append(mob)
                current_id = int(line[1:])
                data = []
            else:
                data.append(line)

    rename(filename, filename + '.bak')
    with open(filename, 'w') as f:
        for mob in mobs:
            m = mob.stats
            f.write(f'#{mob.id}\n')
            f.write(f'{m["namelist"]}~\n')
            f.write(f'{m["short_descr"]}~\n')
            f.write(f'{m["long_descr"]}~\n')
            f.write(f'{m["description"]}~\n')
            f.write(f'{m["mob_flags"].as_ascii()} {m["effect_flags"].as_ascii()} {m["alignment"]} E\n')
            f.write(f'{m["level"]} {m["hitroll"]} {m["ac"]} ')
            f.write(f'{m["hp_num_dice"]}d{m["hp_size_dice"]}+{m["move"]} ')
            f.write(f'{m["extra_dam_num_dice"]}d{m["extra_dam_size_dice"]}+{m["extra_dam_roll_bonus"]}\n')
            f.write(f'{m["gold"]} {m["plat"]} {m["exp"]} {m["zone"]}\n')
            f.write(f'{m["position"]} {m["default_position"]} {m["gender"]} {m["class_num"]} ')
            f.write(f'{m["race"]} {m["race_align"]} {m["size"]}\n')

            for k, v in m.items():
                if k in ['BareHandAttack', 'Str', 'Int', 'Wis', 'Dex', 'Con', 'Cha', 'PERC', 'HIDE', 'Lifeforce',
                         'Composition', 'Stance']:
                    f.write(f'{k}: {v}\n')
                elif k in ['AFF2', 'AFF3', 'MOB2']:
                    f.write(f'{k}: {v.as_ascii()}\n')
            f.write('E\n')
        f.write("$\n")


def process_index(index_file, id):
    path = index_file[:-5]
    with open(index_file) as f:
        for line in f:
            if line.startswith('$'):
                print('End of index')
                return
            elif line.rstrip().endswith('.mob'):
                process_file(path + line.rstrip())


def main(mob_file, id):
    if mob_file.endswith('index'):
        process_index(mob_file, id)
    else:
        process_file(mob_file, id)


if __name__ == '__main__':
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Converts a mob file to use ascii flags.')
    parser.add_argument('--file', help='The mob file to convert or index file to convert all.',
                        type=str, default='../lib/world/mob/index')
    parser.add_argument('-n', '--number', help='The mob number to convert.', type=int, default=-1)
    args = parser.parse_args()
    main(args.file, args.number)
