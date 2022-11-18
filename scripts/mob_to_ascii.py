import argparse


def process_file(filename, vnum=-1):
    print(f"Processing {filename}")
    with open(filename) as f:
        for line in f:
            if line.startswith('#'):
                print(line, end='')


def process_index(index_file, vnum):
    path = index_file[:-5]
    with open(index_file) as f:
        for line in f:
            if line.startswith('$'):
                print('End of index')
                return
            else:
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
                        type=str, default='lib/world/mob/index')
    parser.add_argument('-n', '--number', help='The mob number to convert.', type=int, default=-1)
    args = parser.parse_args()
    main(args.file, args.number)
