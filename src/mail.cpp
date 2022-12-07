/***************************************************************************
 *   File: mail.c                                        Part of FieryMUD  *
 *  Usage: Internal funcs and player spec-procs of mud-mail system         *
 *                                                                         *
 *  Written by Jeremy Elson (jelson@cs.jhu.edu)                            *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "mail.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "modify.hpp"
#include "objects.hpp"
#include "players.hpp"
#include "specprocs.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

const char *RICK_SALA =
    "=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=\n"
    "=~=~=  Rick Sala known to many as his admin character Pergus        =~=~=\n"
    "=~=~=  passed away on June 21, 2006.                                =~=~=\n"
    "=~=~=  Rick was a valuable friend and asset to us all and is sorely =~=~=\n"
    "=~=~=  missed for his wit, humor, and the friend he was to us all.  =~=~=\n"
    "=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=\n";

void postmaster_send_mail(CharData *ch, CharData *mailman, int cmd, char *arg);
void postmaster_check_mail(CharData *ch, CharData *mailman, int cmd, char *arg);
void postmaster_receive_mail(CharData *ch, CharData *mailman, int cmd, char *arg);

void money_convert(CharData *ch, int amount);
int find_name(char *name);

MailIndex *mail_index = 0;   /* list of recs in the mail file  */
PositionList *free_list = 0; /* list of free positions in file */
long file_end_pos = 0;       /* length of file */

void push_free_list(long pos) {
    PositionList *new_pos;

    CREATE(new_pos, PositionList, 1);
    new_pos->position = pos;
    new_pos->next = free_list;
    free_list = new_pos;
}

long pop_free_list(void) {
    PositionList *old_pos;
    long return_value;

    if ((old_pos = free_list) != 0) {
        return_value = free_list->position;
        free_list = old_pos->next;
        free(old_pos);
        return return_value;
    } else
        return file_end_pos;
}

MailIndex *find_char_in_index(long searchee) {
    MailIndex *tmp;

    if (searchee < 0) {
        log("SYSERR: Mail system -- non fatal error #1.");
        return 0;
    }
    for (tmp = mail_index; (tmp && tmp->recipient != searchee); tmp = tmp->next)
        ;

    return tmp;
}

void write_to_file(void *buf, int size, long filepos) {
    FILE *mail_file;

    mail_file = fopen(MAIL_FILE, "r+b");

    if (filepos % BLOCK_SIZE) {
        log("SYSERR: Mail system -- fatal error #2!!!");
        no_mail = 1;
        return;
    }
    fseek(mail_file, filepos, SEEK_SET);
    fwrite(buf, size, 1, mail_file);

    /* find end of file */
    fseek(mail_file, 0L, SEEK_END);
    file_end_pos = ftell(mail_file);
    fclose(mail_file);
    return;
}

void read_from_file(void *buf, int size, long filepos) {
    FILE *mail_file;

    mail_file = fopen(MAIL_FILE, "r+b");

    if (filepos % BLOCK_SIZE) {
        log("SYSERR: Mail system -- fatal error #3!!!");
        no_mail = 1;
        return;
    }
    fseek(mail_file, filepos, SEEK_SET);
    fread(buf, size, 1, mail_file);
    fclose(mail_file);
    return;
}

void index_mail(long id_to_index, long pos) {
    MailIndex *new_index;
    PositionList *new_position;

    if (id_to_index < 0) {
        log("SYSERR: Mail system -- non-fatal error #4.");
        return;
    }
    if (!(new_index = find_char_in_index(id_to_index))) {
        /* name not already in index.. add it */
        CREATE(new_index, MailIndex, 1);
        new_index->recipient = id_to_index;
        new_index->list_start = nullptr;

        /* add to front of list */
        new_index->next = mail_index;
        mail_index = new_index;
    }
    /* now, add this position to front of position list */
    CREATE(new_position, PositionList, 1);
    new_position->position = pos;
    new_position->next = new_index->list_start;
    new_index->list_start = new_position;
}

/* SCAN_FILE */
/* scan_file is called once during boot-up.  It scans through the mail file
   and indexes all entries currently in the mail file. */
int scan_file(void) {
    FILE *mail_file;
    HeaderBlock next_block;
    int total_messages = 0, block_num = 0;
    char buf[100];

    if (!(mail_file = fopen(MAIL_FILE, "r"))) {
        log("Mail file non-existent... creating new file.");
        mail_file = fopen(MAIL_FILE, "w");
        fclose(mail_file);
        return 1;
    }
    while (fread(&next_block, sizeof(HeaderBlock), 1, mail_file)) {
        if (next_block.block_type == HEADER_BLOCK) {
            index_mail(next_block.header_data.to, block_num * BLOCK_SIZE);
            total_messages++;
        } else if (next_block.block_type == DELETED_BLOCK)
            push_free_list(block_num * BLOCK_SIZE);
        block_num++;
    }

    file_end_pos = ftell(mail_file);
    fclose(mail_file);
    sprintf(buf, "   %ld bytes read.", file_end_pos);
    log("%s", buf);
    ;
    if (file_end_pos % BLOCK_SIZE) {
        log("SYSERR: Error booting mail system -- Mail file corrupt!");
        log("SYSERR: Mail disabled!");
        return 0;
    }
    sprintf(buf, "   Mail file read -- %d messages.", total_messages);
    log("%s", buf);
    ;
    return 1;
} /* end of scan_file */

/* HAS_MAIL */
/* a simple little function which tells you if the guy has mail or not */
int has_mail(long recipient) {
    if (find_char_in_index(recipient))
        return 1;
    return 0;
}

/* STORE_MAIL  */
/* call store_mail to store mail.  (hard, huh? :-) )  Pass 3 arguments:
   who the mail is to (long), who it's from (long), and a pointer to the
   actual message text (char *).
*/

/*void store_mail(long to, long from, char *message_pointer)*/
bool store_mail(long to, long from, int vnum, char *message_pointer) {
    HeaderBlock header;
    DataBlock data;
    long last_address, target_address;
    char *msg_txt = message_pointer;
    int bytes_written = 0;
    int total_length = strlen(message_pointer);

    if (sizeof(HEADER_BLOCK) != BLOCK_SIZE || sizeof(HeaderBlock) != sizeof(HEADER_BLOCK)) {
        log("SYSERR: Mail system -- fatal error #4!!!");
        no_mail = 1;
        return false;
    }

    assert(sizeof(HeaderBlock) == sizeof(DataBlock));
    assert(sizeof(HeaderBlock) == BLOCK_SIZE);

    if (from < 0 || to < 0 || !*message_pointer) {
        log("SYSERR: Mail system -- non-fatal error #5.");
        return false;
    }
    memset((char *)&header, 0, sizeof(header)); /* clear the record */
    header.block_type = HEADER_BLOCK;
    header.header_data.next_block = LAST_BLOCK;
    header.header_data.from = from;
    header.header_data.to = to;
    header.header_data.vnum = vnum;
    header.header_data.mail_time = time(0);
    strncpy(header.txt, msg_txt, HEADER_BLOCK_DATASIZE);
    header.txt[HEADER_BLOCK_DATASIZE] = '\0';

    target_address = pop_free_list(); /* find next free block */
    index_mail(to, target_address);   /* add it to mail index in memory */
    write_to_file(&header, BLOCK_SIZE, target_address);

    if (strlen(msg_txt) <= HEADER_BLOCK_DATASIZE)
        return true; /* that was the whole message */

    bytes_written = HEADER_BLOCK_DATASIZE;
    msg_txt += HEADER_BLOCK_DATASIZE; /* move pointer to next bit of text */

    /*
     * find the next block address, then rewrite the header to reflect where
     * the next block is.
     */
    last_address = target_address;
    target_address = pop_free_list();
    header.header_data.next_block = target_address;
    write_to_file(&header, BLOCK_SIZE, last_address);

    /* now write the current data block */
    memset((char *)&data, 0, sizeof(data)); /* clear the record */
    data.block_type = LAST_BLOCK;
    strncpy(data.txt, msg_txt, DATA_BLOCK_DATASIZE);
    data.txt[DATA_BLOCK_DATASIZE] = '\0';
    write_to_file(&data, BLOCK_SIZE, target_address);
    bytes_written += strlen(data.txt);
    msg_txt += strlen(data.txt);

    /*
     * if, after 1 header block and 1 data block there is STILL part of the
     * message left to write to the file, keep writing the new data blocks and
     * rewriting the old data blocks to reflect where the next block is.  Yes,
     * this is kind of a hack, but if the block size is big enough it won't
     * matter anyway.  Hopefully, MUD players won't pour their life stories out
     * into the Mud Mail System anyway.
     *
     * Note that the block_type data field in data blocks is either a number >=0,
     * meaning a link to the next block, or LAST_BLOCK flag (-2) meaning the
     * last block in the current message.  This works much like DOS' FAT.
     */

    while (bytes_written < total_length) {
        last_address = target_address;
        target_address = pop_free_list();

        /* rewrite the previous block to link it to the next */
        data.block_type = target_address;
        write_to_file(&data, BLOCK_SIZE, last_address);

        /* now write the next block, assuming it's the last.  */
        data.block_type = LAST_BLOCK;
        strncpy(data.txt, msg_txt, DATA_BLOCK_DATASIZE);
        data.txt[DATA_BLOCK_DATASIZE] = '\0';
        write_to_file(&data, BLOCK_SIZE, target_address);

        bytes_written += strlen(data.txt);
        msg_txt += strlen(data.txt);
    }
    return true;
} /* store mail */

/* READ_DELETE */
/* read_delete takes 1 char pointer to the name of the person whose mail
you're retrieving.  It returns to you a char pointer to the message text.
The mail is then discarded from the file and the mail index. */

/*char *read_delete(long recipient)*/
char *read_delete(long recipient, int *obj_vnum)
/* recipient is the name as it appears in the index.
   recipient_formatted is the name as it should appear on the mail
   header (i.e. the text handed to the player) */
{
    HeaderBlock header;
    DataBlock data;
    MailIndex *mail_pointer, *prev_mail;
    PositionList *position_pointer;
    long mail_address, following_block;
    char *message, buf[200];
    size_t string_size;

    if (recipient < 0) {
        log("SYSERR: Mail system -- non-fatal error #6.");
        return 0;
    }
    if (!(mail_pointer = find_char_in_index(recipient))) {
        log("SYSERR: Mail system -- post office spec_proc error?  Error #7.");
        return 0;
    }
    if (!(position_pointer = mail_pointer->list_start)) {
        log("SYSERR: Mail system -- non-fatal error #8.");
        return 0;
    }
    if (!(position_pointer->next)) { /* just 1 entry in list. */
        mail_address = position_pointer->position;
        free(position_pointer);

        /* now free up the actual name entry */
        if (mail_index == mail_pointer) { /* name is 1st in list */
            mail_index = mail_pointer->next;
            free(mail_pointer);
        } else {
            /* find entry before the one we're going to del */
            for (prev_mail = mail_index; prev_mail->next != mail_pointer; prev_mail = prev_mail->next)
                ;
            prev_mail->next = mail_pointer->next;
            free(mail_pointer);
        }
    } else {
        /* move to next-to-last record */
        while (position_pointer->next->next)
            position_pointer = position_pointer->next;
        mail_address = position_pointer->next->position;
        free(position_pointer->next);
        position_pointer->next = 0;
    }

    /* ok, now lets do some readin'! */
    read_from_file(&header, BLOCK_SIZE, mail_address);

    if (header.block_type != HEADER_BLOCK) {
        log("SYSERR: Oh dear.");
        no_mail = 1;
        log("SYSERR: Mail system disabled!  -- Error #9.");
        return 0;
    }
    strftime(buf1, 15, TIMEFMT_DATE, localtime(&header.header_data.mail_time));
    *obj_vnum = header.header_data.vnum;

    sprintf(buf,
            " * * * * &2FieryMUD Mail System&0 * * * *\n"
            "Date: %s\n"
            "  To: %s\n"
            "From: %s\n\n",
            buf1, get_name_by_id(recipient), get_name_by_id(header.header_data.from));

    string_size = (sizeof(char) * (strlen(buf) + strlen(header.txt) + 3));
    CREATE(message, char, string_size);
    strcpy(message, buf);
    strcat(message, header.txt);
    strcat(message, "@0");
    message[string_size - 1] = '\0';
    following_block = header.header_data.next_block;

    /* mark the block as deleted */
    header.block_type = DELETED_BLOCK;
    write_to_file(&header, BLOCK_SIZE, mail_address);
    push_free_list(mail_address);

    while (following_block != LAST_BLOCK) {
        read_from_file(&data, BLOCK_SIZE, following_block);

        string_size = (sizeof(char) * (strlen(message) + strlen(data.txt) + 1));
        RECREATE(message, char, string_size);
        strcat(message, data.txt);
        message[string_size - 1] = '\0';
        mail_address = following_block;
        following_block = data.block_type;
        data.block_type = DELETED_BLOCK;
        write_to_file(&data, BLOCK_SIZE, mail_address);
        push_free_list(mail_address);
    }

    return message;
}

/*******************************************************************
 ** Below is the spec_proc for a postmaster using the above       **
 ** routines.  Written by Jeremy Elson (jelson@server.cs.jhu.edu) **
 *******************************************************************/

SPECIAL(postmaster) {
    if (!ch->desc || IS_NPC(ch))
        return 0; /* so mobs don't get caught here */

    if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive")))
        return 0;

    if (no_mail) {
        send_to_char("Sorry, the mail system is having technical difficulties.\n", ch);
        return 1;
    }

    if (CMD_IS("mail")) {
        postmaster_send_mail(ch, (CharData *)me, cmd, argument);
        return 1;
    } else if (CMD_IS("check")) {
        postmaster_check_mail(ch, (CharData *)me, cmd, argument);
        return 1;
    } else if (CMD_IS("receive")) {
        postmaster_receive_mail(ch, (CharData *)me, cmd, argument);
        return 1;
    } else
        return 0;
}

void postmaster_send_mail(CharData *ch, CharData *mailman, int cmd, char *arg) {
    long recipient;
    char buf[256];
    char buf2[256];
    int price = STAMP_PRICE;
    ObjData *obj;

    obj = nullptr;

    if (GET_LEVEL(ch) < MIN_MAIL_LEVEL) {
        sprintf(buf, "$n tells you, 'Sorry, you have to be level %d to send mail!'", MIN_MAIL_LEVEL);
        act(buf, false, mailman, 0, ch, TO_VICT);
        return;
    }
    /*one_argument(arg, buf); */
    two_arguments(arg, buf, buf2);

    if (!*buf) { /* you'll get no argument from me! */
        act("$n tells you, 'You need to specify an addressee!'", false, mailman, 0, ch, TO_VICT);
        return;
    }
    /* =~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
       Code to let players know if another player has passed away and
       not generate any mail for them. - RSD 6/24/2006
       =~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
     */
    if (strcasecmp(buf, "pergus") == 0) {
        act("$n tells you, 'I'm sorry, that character has passed away....'", false, mailman, 0, ch, TO_VICT);
        act(RICK_SALA, false, mailman, 0, ch, TO_VICT);
        return;
    }
    if (strcasecmp(buf, "daroowise") == 0) {
        act("$n tells you, 'I'm sorry, that character has passed away....'", false, mailman, 0, ch, TO_VICT);
        act(RICK_SALA, false, mailman, 0, ch, TO_VICT);
        return;
    }
    if (strcasecmp(buf, "ninmei") == 0) {
        act("$n tells you, 'I'm sorry, that character has passed away....'", false, mailman, 0, ch, TO_VICT);
        act(RICK_SALA, false, mailman, 0, ch, TO_VICT);
        return;
    }
    if (strcasecmp(buf, "brilan") == 0) {
        act("$n tells you, 'I'm sorry, that character has passed away....'", false, mailman, 0, ch, TO_VICT);
        act(RICK_SALA, false, mailman, 0, ch, TO_VICT);
        return;
    }

    if (*buf2 && (obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, buf2)))) {
        if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
            act("You can't mail $p.  It's CURSED!", false, ch, obj, obj, TO_CHAR);
            return;
        }
        if (OBJ_FLAGGED(obj, ITEM_NORENT)) {
            act("$n tells you, 'I'm not sending that!'", false, mailman, 0, ch, TO_VICT);
            return;
        }
        price += STAMP_PRICE * .1;
        price += GET_OBJ_EFFECTIVE_WEIGHT(obj) * 100;
    } else if (*buf2) {
        char_printf(ch, "You don't seem to have a %s to mail.\n", buf2);
        return;
    }

    if (GET_CASH(ch) < price) {
        if (GET_LEVEL(ch) < 100) {
            sprintf(buf,
                    "$n tells you, 'A stamp costs %d copper coins.'\n"
                    "$n tells you, '...which I see you can't afford.'",
                    price);
            act(buf, false, mailman, 0, ch, TO_VICT);
            return;
        } else {
            price = 0;
        }
    }

    if (GET_LEVEL(ch) >= 100)
        price = 0;

    if ((recipient = get_id_by_name(buf)) < 0) {
        act("$n tells you, 'No one by that name is registered here!'", false, mailman, 0, ch, TO_VICT);
        return;
    }

    if (obj != nullptr) {
        ch->desc->mail_vnum = GET_OBJ_VNUM(obj);
        act("$n takes $p and prepares it for packaging.", false, mailman, obj, ch, TO_VICT);
        extract_obj(obj);
    } else
        ch->desc->mail_vnum = NOTHING;

    act("$n starts to write some mail.", true, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 100)
        sprintf(buf,
                "$n tells you, 'I'll take %d coins for the stamp.'\n"
                "$n tells you, 'Write your message, (/s saves /h for help)'",
                price);
    else
        sprintf(buf,
                "$n tells you, 'I refuse to take money from a deity.  This "
                "stamp is on me.'\n"
                "$n tells you, 'Write your message, (/s saves /h for help)'");

    act(buf, false, mailman, 0, ch, TO_VICT);
    money_convert(ch, price);
    GET_COPPER(ch) -= price;
    SET_FLAG(PLR_FLAGS(ch), PLR_MAILING);
    SET_FLAG(PLR_FLAGS(ch), PLR_WRITING);

    mail_write(ch->desc, nullptr, MAX_MAIL_SIZE, recipient);
}

void postmaster_check_mail(CharData *ch, CharData *mailman, int cmd, char *arg) {
    char buf[256];

    if (has_mail(GET_IDNUM(ch)))
        sprintf(buf, "$n tells you, 'You have mail waiting.'");
    else
        sprintf(buf, "$n tells you, 'Sorry, you don't have any mail waiting.'");
    act(buf, false, mailman, 0, ch, TO_VICT);
}

void postmaster_receive_mail(CharData *ch, CharData *mailman, int cmd, char *arg) {
    char buf[256];
    /* ObjData *obj; */
    ObjData *obj, *mail_obj;
    int obj_vnum = NOTHING;

    if (!has_mail(GET_IDNUM(ch))) {
        sprintf(buf, "$n tells you, 'Sorry, you don't have any mail waiting.'");
        act(buf, false, mailman, 0, ch, TO_VICT);
        return;
    }
    while (has_mail(GET_IDNUM(ch))) {
        obj = create_obj();
        obj->item_number = NOTHING;
        obj->name = strdup("mail paper letter");
        obj->short_description = strdup("a piece of mail");
        obj->description = strdup("Someone has left a piece of mail here.");

        GET_OBJ_TYPE(obj) = ITEM_NOTE;
        GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE | ITEM_WEAR_HOLD;
        GET_OBJ_WEIGHT(obj) = GET_OBJ_EFFECTIVE_WEIGHT(obj) = 1;
        GET_OBJ_COST(obj) = 30;
        /* GET_OBJ_RENT(obj) = 10; */
        obj->action_description = read_delete(GET_IDNUM(ch), &obj_vnum);

        if (obj->action_description == nullptr)
            obj->action_description = strdup("Mail system error - please report.  Error #11.\n");

        obj_to_char(obj, ch);

        if (obj_vnum != NOTHING && real_object(obj_vnum) != NOTHING) {
            mail_obj = read_object(real_object(obj_vnum), REAL);
            obj_to_char(mail_obj, ch);
            act("$n gives you $p, which was attached to your mail.", false, mailman, mail_obj, ch, TO_VICT);
            act("$N gives $n $p, which was attached to $S mail.", false, ch, mail_obj, mailman, TO_ROOM);
        }

        act("$n gives you a piece of mail.", false, mailman, 0, ch, TO_VICT);
        act("$N gives $n a piece of mail.", false, ch, 0, mailman, TO_ROOM);
    }
}

void free_mail_index(void) {
    MailIndex *next_mail;
    PositionList *next_pos;

    extern MailIndex *mail_index;
    extern PositionList *free_list;

    while (mail_index) {
        next_mail = mail_index->next;
        while (mail_index->list_start) {
            next_pos = mail_index->list_start->next;
            free(mail_index->list_start);
            mail_index->list_start = next_pos;
        }
        free(mail_index);
        mail_index = next_mail;
    }

    while (free_list) {
        next_pos = free_list->next;
        free(free_list);
        free_list = next_pos;
    }
}
