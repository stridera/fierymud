class Mob:
    def __init__(self, vnum, namelist, short_desc, long_desc, description, act, affected_by, alignment, level, hitroll, ac, hit, mana, damage):
        self.vnum = vnum
        self.namelist = namelist
        self.short_desc = short_desc
        self.long_desc = long_desc
        self.description = description
        self.act = act
        self.affected_by = affected_by
        self.alignment = alignment
        self.level = level
        self.hitroll = hitroll
        self.ac = ac
        self.hit = hit
        self.mana = mana
        self.damage = damage


"""

    mob_index[i].virtual = nr;
    mob_proto[i].player.namelist = fread_string(mob_f, buf2);
    tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
    mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
    mob_proto[i].player.description = fread_string(mob_f, buf2);

    /* *** Numeric data *** */
    get_line(mob_f, line);
    sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter);
    MOB_FLAGS(mob_proto + i)[0] = asciiflag_conv(f1);
    EFF_FLAGS(mob_proto + i)[0] = asciiflag_conv(f2);
    GET_ALIGNMENT(mob_proto + i) = t[2];

    switch (letter) {
    case 'S': /* Simple monsters */
        parse_simple_mob(mob_f, i, nr);
        break;
    case 'E': /* Circle3 Enhanced monsters */
        parse_enhanced_mob(mob_f, i, nr);
        break;
        /* add new mob types here.. */
    default:
        fprintf(stderr, "Unsupported mob type '%c' in mob #%d\n", letter, nr);
        exit(1);
        break;
    }

    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
    while (letter == 'T') {
        dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
        letter = fread_letter(mob_f);
        ungetc(letter, mob_f);
    }

    mob_proto[i].affected_abils = mob_proto[i].natural_abils;

    for (j = 0; j < NUM_WEARS; j++)
        mob_proto[i].equipment[j] = NULL;

    mob_proto[i].desc = NULL;

    letter = fread_letter(mob_f);
    if (letter == '>') {
        while (fread_letter(mob_f) != '|')
            ;
        fprintf(stderr, "Mob %d has a mobprog still!\n", nr);
    } else
        ungetc(letter, mob_f);

    if (mob_proto[i].mob_specials.default_pos < 0 || mob_proto[i].mob_specials.default_pos >= NUM_POSITIONS) {
        mob_proto[i].mob_specials.default_pos = POS_STANDING;
    }
    if (mob_proto[i].char_specials.position < 0 || mob_proto[i].char_specials.position >= NUM_POSITIONS) {
        mob_proto[i].char_specials.position = POS_STANDING;
    }
"""