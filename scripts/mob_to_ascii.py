






def main(mob_file, vnum, output_file):
    if mob_file.endswith('index'):
        process_index(mob_file, vnum, output_file)
    else:
        process_file(mob_file, vnum)


if __name__ == '__main__':
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Converts a mob file to use ascii flags.')
    parser.add_argument('mob_file', help='The mob file to convert or index file to convert all.', default='lib/')
    parser.add_argument('-n', '--number', help='The mob number to convert.', type=int)
    parser.add_argument('-v', '--verbose', help='Verbose output.', action='store_true')
    args = parser.parse_args()
    main(args.mob_file, args.number, args.verbose)

    