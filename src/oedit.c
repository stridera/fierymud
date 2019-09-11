/***************************************************************************
 * $Id: oedit.c,v 1.58 2009/03/20 20:19:51 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: oedit.c                                        Part of FieryMUD *
 *  Usage:                                                                 *
 *     By: Levork (whoever that is)                                        *
 *         TwyliteMud by Rv. (shameless plug)                              *
 *         Copyright 1996 Harvey Gilpin.                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "casting.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "olc.h"
#include "dg_olc.h"
#include "skills.h"
#include "constants.h"
#include "math.h"
#include "fight.h"
#include "interpreter.h"
#include "handler.h"
#include "modify.h"
#include "board.h"
#include "composition.h"

/* external variables */
int *tmp_ptr;
extern const char *portal_entry_messages[];
extern const char *portal_character_messages[];
extern const char *portal_exit_messages[];

/*------------------------------------------------------------------------*/
/*. Macros .*/

#define S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*/

void oedit_disp_container_flags_menu(struct descriptor_data *d);
void oedit_disp_extradesc_menu(struct descriptor_data *d);
void oedit_disp_weapon_menu(struct descriptor_data *d);
void oedit_disp_val1_menu(struct descriptor_data *d);
void oedit_disp_val2_menu(struct descriptor_data *d);
void oedit_disp_val3_menu(struct descriptor_data *d);
void oedit_disp_val4_menu(struct descriptor_data *d);
void oedit_disp_type_menu(struct descriptor_data *d);
void oedit_disp_extra_menu(struct descriptor_data *d);
void oedit_disp_wear_menu(struct descriptor_data *d);
void oedit_disp_menu(struct descriptor_data *d);

void oedit_parse(struct descriptor_data *d, char *arg);
void oedit_disp_spells_menu(struct descriptor_data *d);
void oedit_liquid_type(struct descriptor_data *d);
void oedit_setup_new(struct descriptor_data *d);
void oedit_setup_existing(struct descriptor_data *d, int real_num);
void oedit_save_to_disk(int zone);
void oedit_reverse_exdescs(int zone, struct char_data *ch);
int oedit_reverse_exdesc(int real_num, struct char_data *ch);
void oedit_save_internally(struct descriptor_data *d);
void iedit_save_changes(struct descriptor_data *d);

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

void oedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_OBJ(d), struct obj_data, 1);
  clear_object(OLC_OBJ(d));
  OLC_OBJ(d)->name = strdup("unfinished object");
  OLC_OBJ(d)->description = strdup("An unfinished object is lying here.");
  OLC_OBJ(d)->short_description = strdup("an unfinished object");
  GET_OBJ_WEAR(OLC_OBJ(d)) = ITEM_WEAR_TAKE;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  OLC_VAL(d) = 0;
  oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void oedit_setup_existing(struct descriptor_data *d, int real_num)
{
  struct extra_descr_data *this, *temp, *temp2;
  struct obj_data *obj;

  /*
   * Allocate object in memory.
   */
  CREATE(obj, struct obj_data, 1);

  clear_object(obj);
  *obj = obj_proto[real_num];


  /*
   * Copy all strings over.
   */
  obj->name = strdup(obj_proto[real_num].name ? obj_proto[real_num].name : "undefined");
  obj->short_description = strdup(obj_proto[real_num].short_description ?
                   obj_proto[real_num].short_description : "undefined");
  obj->description = strdup(obj_proto[real_num].description ?
                 obj_proto[real_num].description : "undefined");
  obj->action_description = (obj_proto[real_num].action_description ?
                 strdup(obj_proto[real_num].action_description) : NULL);

  /*
   * Extra descriptions if necessary.
   */
  if (obj_proto[real_num].ex_description) {
    CREATE(temp, struct extra_descr_data, 1);

    obj->ex_description = temp;
    for (this = obj_proto[real_num].ex_description; this; this = this->next) {
      temp->keyword = (this->keyword && *this->keyword) ? strdup(this->keyword) : NULL;
      temp->description = (this->description && *this->description) ?
    strdup(this->description) : NULL;
      if (this->next) {
    CREATE(temp2, struct extra_descr_data, 1);
    temp->next = temp2;
    temp = temp2;
      } else
    temp->next = NULL;
    }
  }

  /*. Attatch new obj to players descriptor .*/
  OLC_OBJ(d) = obj;
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  dg_olc_script_copy(d);
  oedit_disp_menu(d);
}
/*------------------------------------------------------------------------*/

#define ZCMD zone_table[zone].cmd[cmd_no]

void oedit_save_internally(struct descriptor_data *d)
{
  int i, shop, robj_num, found = FALSE, zone, cmd_no;
  struct extra_descr_data *this, *next_one;
  struct obj_data *obj, *swap, *new_obj_proto;
  struct index_data *new_obj_index;
  struct descriptor_data *dsc;

  /*
   * Write object to internal tables.
   */
  if ((robj_num = real_object(OLC_NUM(d))) > 0) {
    /*
     * Are we purging?
     */
    if (OLC_MODE(d) == OEDIT_PURGE_OBJECT)
    {
      struct obj_data *placeholder;
      void extract_obj(struct obj_data * ch); /* leaves eq behind..*/

      for (obj = object_list; obj; obj = obj->next)
      {
        if ( GET_OBJ_RNUM(obj) == robj_num)
    {
        placeholder=obj;
#if defined(DEBUG)
        fprintf(stderr, "remove object %d ",GET_OBJ_VNUM(obj));
#endif
        extract_obj(obj);    /*remove all existing objects*/
        obj=placeholder;    /*so we can keep removing..*/
#if defined(DEBUG)
        fprintf(stderr,"(%d left)\n",obj_index[robj_num].number);
#endif
    }
      }
    CREATE(new_obj_proto, struct obj_data, top_of_objt );
    CREATE(new_obj_index, struct index_data, top_of_objt );
#if defined(DEBUG)
    fprintf(stderr,"looking to destroy %d (%d)\n",robj_num,obj_index[robj_num].virtual);
#endif
        for (i = 0; i <= top_of_objt ; i++)
    {
          if (!found)
      {        /* Is this the place? */
    /*    if ((robj_num > top_of_objt) || (mob_index[robj_num].virtual > OLC_NUM(d))) */
        if (i == robj_num)
        {
          found = TRUE;
          /* don't copy..it will be blatted by the free later*/
        }
        else
        {    /* Nope, copy over as normal. */
          new_obj_index[i] = obj_index[i];
          new_obj_proto[i] = obj_proto[i];
        }
          }
      else
      { /* We've already found it, copy the rest over. */
        new_obj_index[i - 1] = obj_index[i];
        new_obj_proto[i - 1] = obj_proto[i];
          }
    }
        top_of_objt --;
#if !defined(I_CRASH)
        free(obj_index);
        free(obj_proto);
#endif
        obj_index = new_obj_index;
        obj_proto = new_obj_proto;
    /*. Renumber live objects .*/
    for (obj = object_list; obj; obj = obj->next)
      if (GET_OBJ_RNUM(obj) != NOTHING && GET_OBJ_RNUM (obj) >= robj_num)
        GET_OBJ_RNUM (obj)--;

    /*. Renumber zone table .*/
    for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
      switch (ZCMD.command) {
    case 'P':
      if (ZCMD.arg3 >= robj_num)
        ZCMD.arg3--;
      /*
       * No break here - drop into next case.
       */
    case 'O':
    case 'G':
    case 'E':
      if (ZCMD.arg1 >= robj_num)
        ZCMD.arg1--;
      break;
    case 'R':
      if (ZCMD.arg2 >= robj_num)
        ZCMD.arg2--;
      break;
    }

    /*. Renumber shop produce .*/
    for(shop = 0; shop < top_shop; shop++)
      for(i = 0; SHOP_PRODUCT(shop, i) != -1; i++)
        if (SHOP_PRODUCT(shop, i) >= robj_num)
          SHOP_PRODUCT(shop, i)--;

    /*and those being edited     */
    for (dsc = descriptor_list; dsc; dsc = dsc->next)
      if (dsc->connected == CON_SEDIT)
    for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
      if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
        S_PRODUCT(OLC_SHOP(dsc), i)--;

      /*MARK POINT XXXXX*/
    } else {
    /*
     * We need to run through each and every object currently in the
     * game to see which ones are pointing to this prototype.
     * if object is pointing to this prototype, then we need to replace it
     * with the new one.
     */
    CREATE(swap, struct obj_data, 1);
    for (obj = object_list; obj; obj = obj->next) {
      if (obj->item_number == robj_num) {
    *swap = *obj;
    *obj = *OLC_OBJ(d);
    /*
     * Copy game-time dependent variables over.
     */
    obj->in_room = swap->in_room;
    obj->item_number = robj_num;
    obj->carried_by = swap->carried_by;
    obj->worn_by = swap->worn_by;
    obj->worn_on = swap->worn_on;
    obj->in_obj = swap->in_obj;
    obj->contains = swap->contains;
    obj->next_content = swap->next_content;
    obj->next = swap->next;
        obj->proto_script = OLC_SCRIPT(d);
      }
    }
    free(swap);
    /* now safe to free old proto and write over */
    if (obj_proto[robj_num].name)
      free(obj_proto[robj_num].name);
    if (obj_proto[robj_num].description)
      free(obj_proto[robj_num].description);
    if (obj_proto[robj_num].short_description)
      free(obj_proto[robj_num].short_description);
    if (obj_proto[robj_num].action_description)
      free(obj_proto[robj_num].action_description);
    if (obj_proto[robj_num].ex_description)
      for (this = obj_proto[robj_num].ex_description; this; this = next_one) {
    next_one = this->next;
    if (this->keyword)
      free(this->keyword);
    if (this->description)
      free(this->description);
    free(this);
      }
    /* Must do this before copying OLC_OBJ over */
    if (obj_proto[robj_num].proto_script &&
        obj_proto[robj_num].proto_script != OLC_SCRIPT(d))
      free_proto_script(&obj_proto[robj_num].proto_script);
    obj_proto[robj_num] = *OLC_OBJ(d);
    obj_proto[robj_num].item_number = robj_num;
    obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
  }
  } else {
    /*. It's a new object, we must build new tables to contain it .*/

    CREATE(new_obj_index, struct index_data, top_of_objt + 2);
    CREATE(new_obj_proto, struct obj_data, top_of_objt + 2);
    /* start counting through both tables */
    for (i = 0; i <= top_of_objt; i++) {
      /* if we haven't found it */
      if (!found) {
    /*
     * Check if current virtual is bigger than our virtual number.
     */
    if (obj_index[i].virtual > OLC_NUM(d)) {
      found = TRUE;
      robj_num = i;
      OLC_OBJ(d)->item_number = robj_num;
      new_obj_index[robj_num].virtual = OLC_NUM(d);
      new_obj_index[robj_num].number = 0;
      new_obj_index[robj_num].func = NULL;
      new_obj_proto[robj_num] = *(OLC_OBJ(d));
      new_obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
      new_obj_proto[robj_num].in_room = NOWHERE;
          /*. Copy over the mob that should be here .*/
          new_obj_index[robj_num + 1] = obj_index[robj_num];
      new_obj_proto[robj_num + 1] = obj_proto[robj_num];
      new_obj_proto[robj_num + 1].item_number = robj_num + 1;
        } else {
      /* just copy from old to new, no num change */
      new_obj_proto[i] = obj_proto[i];
        new_obj_index[i] = obj_index[i];
    }
      } else {
        /* we HAVE already found it.. therefore copy to object + 1 */
        new_obj_index[i + 1] = obj_index[i];
        new_obj_proto[i + 1] = obj_proto[i];
        new_obj_proto[i + 1].item_number = i + 1;
      }
    }
    if (!found) {
      robj_num = i;
      OLC_OBJ(d)->item_number = robj_num;
      new_obj_index[robj_num].virtual = OLC_NUM(d);
      new_obj_index[robj_num].number = 0;
      new_obj_index[robj_num].func = NULL;
      new_obj_proto[robj_num] = *(OLC_OBJ(d));
      new_obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
      new_obj_proto[robj_num].in_room = NOWHERE;
    }

    /* free and replace old tables */
    free (obj_proto);
    free (obj_index);
    obj_proto = new_obj_proto;
    obj_index = new_obj_index;
    top_of_objt++;

    /*. Renumber live objects .*/
    for (obj = object_list; obj; obj = obj->next)
      if (GET_OBJ_RNUM(obj) != NOTHING && GET_OBJ_RNUM (obj) >= robj_num)
        GET_OBJ_RNUM (obj)++;



    /*. Renumber zone table .*/
    for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
      switch (ZCMD.command) {
    case 'P':
      if (ZCMD.arg3 >= robj_num)
        ZCMD.arg3++;
      /*
       * No break here - drop into next case.
       */
    case 'O':
    case 'G':
    case 'E':
      if (ZCMD.arg1 >= robj_num)
        ZCMD.arg1++;
      break;
    case 'R':
      if (ZCMD.arg2 >= robj_num)
        ZCMD.arg2++;
      break;
    }

    /*. Renumber shop produce .*/
    for(shop = 0; shop < top_shop; shop++)
      for(i = 0; SHOP_PRODUCT(shop, i) != -1; i++)
        if (SHOP_PRODUCT(shop, i) >= robj_num)
          SHOP_PRODUCT(shop, i)++;




    /*and those being edited     */
    for (dsc = descriptor_list; dsc; dsc = dsc->next)
      if (dsc->connected == CON_SEDIT)
    for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
      if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
        S_PRODUCT(OLC_SHOP(dsc), i)++;

  }
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_OBJ);
}

/*------------------------------------------------------------------------*/


void oedit_save_to_disk(int zone_num)
{
  int counter, counter2, realcounter;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

  sprintf(buf, "%s/%d.new", OBJ_PREFIX, zone_table[zone_num].number);
  if (!(fp = fopen(buf, "w+"))) {
    mudlog("SYSERR: OLC: Cannot open objects file!", BRF, LVL_GOD,
       TRUE);
    return;
  }
  /*
   * Start running through all objects in this zone.
   */
  for (counter = zone_table[zone_num].number * 100;
       counter <= zone_table[zone_num].top; counter++) {
    if ((realcounter = real_object(counter)) >= 0) {
      if ((obj = (obj_proto + realcounter))->action_description) {
    strcpy(buf1, obj->action_description);
    strip_string(buf1);
      } else
    *buf1 = '\0';

    fprintf(fp,
        "#%d\n"
        "%s~\n"
        "%s~\n"
        "%s~\n"
        "%s~\n"
        "%d %ld %d %d\n"
        "%d %d %d %d %d %d %d\n"
        "%.2f %d %d %ld %d %d %ld %ld\n",

        GET_OBJ_VNUM(obj),
        (obj->name && *obj->name) ? obj->name : "undefined",
        (obj->short_description && *obj->short_description) ?
        obj->short_description : "undefined",
        (obj->description && *obj->description) ?
        obj->description : "undefined",
        buf1,

        GET_OBJ_TYPE(obj), GET_OBJ_FLAGS(obj)[0],
        GET_OBJ_WEAR(obj), GET_OBJ_LEVEL(obj),

        GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
        GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4), GET_OBJ_VAL(obj, 5),
        GET_OBJ_VAL(obj, 6),

        GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj),
        GET_OBJ_TIMER(obj), GET_OBJ_EFF_FLAGS(obj)[0],
        0, 0,
        GET_OBJ_EFF_FLAGS(obj)[1], GET_OBJ_EFF_FLAGS(obj)[2]
        );

      script_save_to_disk(fp, obj, OBJ_TRIGGER);

      /*
       * Do we have extra descriptions?
       */
      if (obj->ex_description) {    /* Yes, save them too. */
    for (ex_desc = obj->ex_description; ex_desc; ex_desc =
           ex_desc->next) {
      /*
       * Sanity check to prevent nasty protection faults.
       */
      if (!*ex_desc->keyword || !*ex_desc->description) {
        mudlog("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!",
           BRF, LVL_GOD, TRUE);
        continue;
      }
      strcpy(buf1, ex_desc->description);
      strip_string(buf1);
      fprintf(fp, "E\n"
          "%s~\n"
          "%s~\n", ex_desc->keyword, buf1);
    }
      }

      /* Do we have affects? */
      for (counter2 = 0; counter2 < MAX_OBJ_APPLIES; counter2++)
          if (obj->applies[counter2].location)
      fprintf(fp, "A\n"
          "%d %d\n", obj->applies[counter2].location,
          obj->applies[counter2].modifier);

      if (obj->obj_flags.hiddenness)
        fprintf(fp, "H\n%ld\n", obj->obj_flags.hiddenness);
    }
  }

  /* write final line, close */
  fprintf(fp, "$~\n");
  fclose(fp);
  sprintf(buf2, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
  remove(buf2);
  rename(buf, buf2);

  olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_OBJ);
}


/*------------------------------------------------------------------------*/

int oedit_reverse_exdesc(int real_num, struct char_data *ch)
{
   struct obj_data *obj;
   struct extra_descr_data *ex_desc, *tmp;

   obj = obj_proto + real_num;

   if ((ex_desc = obj->ex_description) && ex_desc->next) {
      for (obj->ex_description = NULL; ex_desc;) {
         tmp = ex_desc->next;
         ex_desc->next = obj->ex_description;
         obj->ex_description = ex_desc;
         ex_desc = tmp;
      }
      if (ch) {
         sprintf(buf, "Reversed exdescs of object %d, %s.\r\n",
               GET_OBJ_VNUM(obj), obj->short_description);
         send_to_char(buf, ch);
      }
      olc_add_to_save_list(
            zone_table[find_real_zone_by_room(GET_OBJ_VNUM(obj))].number,
            OLC_SAVE_OBJ);
      return 1;
   }
   return 0;
}

/*------------------------------------------------------------------------*/

void oedit_reverse_exdescs(int zone, struct char_data *ch)
{
   int counter, realcounter, numprocessed = 0, nummodified = 0;

   for (counter = zone_table[zone].number * 100;
         counter <= zone_table[zone].top; counter++) {
      if ((realcounter = real_object(counter)) >= 0) {
         nummodified += oedit_reverse_exdesc(realcounter, ch);
         numprocessed++;
      }
   }

   /*
   if (nummodified)
      olc_add_to_save_list(zone_table[zone].number, OLC_SAVE_OBJ);
      */

   if (ch) {
      sprintf(buf, "Modified %d of %d object prototype%s in zone %d, %s.\r\n",
            nummodified, numprocessed, numprocessed == 1 ? "" : "s",
            zone_table[zone].number, zone_table[zone].name);
      send_to_char(buf, ch);
   }
}

/**************************************************************************
 *                         Menu functions                                 *
 **************************************************************************/

/*
 *  For wall flags.
 */
void oedit_disp_wall_block_dirs(struct descriptor_data *d)
{
  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("^[[H^[[J", d->character);
#endif
  sprintf(buf,
      "%s0%s) NORTH\r\n"
      "%s1%s) EAST\r\n"
      "%s2%s) SOUTH\r\n"
      "%s3%s) WEST\r\n"
      "%s4%s) UP\r\n"
      "%s5%s) DOWN\r\n"
      "Enter flag : ",
      grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm);
  send_to_char(buf, d->character);
}

/*
 * For container flags.
 */
void oedit_disp_container_flags_menu(struct descriptor_data *d)
{
  get_char_cols(d->character);
  sprintbit(GET_OBJ_VAL(OLC_OBJ(d), VAL_CONTAINER_BITS), container_bits, buf1);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  sprintf(buf,
      "%s1%s) CLOSEABLE\r\n"
      "%s2%s) PICKPROOF\r\n"
      "%s3%s) CLOSED\r\n"
      "%s4%s) LOCKED\r\n"
      "Container flags: %s%s%s\r\n"
      "Enter flag, 0 to quit : ",
      grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

/*
 * For extra descriptions.
 */
void oedit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  strcpy(buf1, !extra_desc->next ? "<Not set>\r\n" : "Set.");

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  sprintf(buf,
      "Extra desc menu\r\n"
      "%s1%s) Keyword: %s%s\r\n"
      "%s2%s) Description:\r\n%s%s\r\n"
      "%s3%s) Goto next description: %s\r\n"
      "%s0%s) Quit\r\n"
      "Enter choice : ",

           grn, nrm, yel, (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>",
      grn, nrm, yel, (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>",
      grn, nrm, buf1, grn, nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/*
 * Ask for *which* apply to edit.
 */
void oedit_disp_prompt_apply_menu(struct descriptor_data *d)
{
  int counter;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; counter < MAX_OBJ_APPLIES; counter++) {
    sprintf(buf, " %s%d%s) %s\r\n", grn, counter + 1, nrm,
          format_apply(OLC_OBJ(d)->applies[counter].location,
               OLC_OBJ(d)->applies[counter].modifier));
    send_to_char(buf, d->character);
  }
  send_to_char("\r\nEnter apply to modify (0 to quit) : ", d->character);
  OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

#define FLAG_INDEX      ((NUM_EFF_FLAGS / columns + 1) * j + i)
void oedit_disp_aff_flags(struct descriptor_data *d)
{
  const int columns = 3;
  int i, j;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char(".[H.[J", d->character);
#endif

  for (i = 0; i <= NUM_EFF_FLAGS / columns; ++i) {
    *buf = '\0';
    for (j = 0; j < columns; ++j) {
      if (FLAG_INDEX >= NUM_EFF_FLAGS)
        break;
      sprintf(buf, "%s%s%2d%s) %-20.20s", buf, grn, FLAG_INDEX + 1, nrm, effect_flags[FLAG_INDEX]);
    }
    send_to_char(strcat(buf, "\r\n"), d->character);
  }

  *buf1 = '\0';
  sprintflag(buf1, GET_OBJ_EFF_FLAGS(OLC_OBJ(d)), NUM_EFF_FLAGS, effect_flags);
  sprintf(buf, "\r\nSpell flags: %s%s%s\r\n"
      "Enter spell flag, 0 to quit : ", cyn, buf1, nrm);
  send_to_char(buf, d->character);
}
#undef FLAG_INDEX


void oedit_disp_component(struct descriptor_data *d)
{
   /*  int counter; */

  get_char_cols(d->character);
  send_to_char("this is not a functional system yet sorry", d->character);

}

/*
 * Ask for liquid type.
 */
#define TYPE_INDEX    ((NUM_LIQ_TYPES / columns + 1) * j + i)
void oedit_liquid_type(struct descriptor_data *d)
{
  const int columns = 3;
  int i, j;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif

  for (i = 0; i <= NUM_LIQ_TYPES / columns; ++i) {
    *buf = '\0';
    for (j = 0; j < columns; ++j)
      if (TYPE_INDEX < NUM_LIQ_TYPES)
        sprintf(buf, "%s%s%2d%s) %s%-20.20s%s ", buf,
                grn, TYPE_INDEX + 1, nrm, yel, LIQ_NAME(TYPE_INDEX), nrm);
    send_to_char(strcat(buf, "\r\n"), d->character);
  }

  sprintf(buf, "\r\n%sEnter drink type : ", nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_VALUE_3;
}
#undef TYPE_INDEX

/*
 * The actual apply to set.
 */
#define TYPE_INDEX    ((NUM_APPLY_TYPES / columns + 1) * j + i)
void oedit_disp_apply_menu(struct descriptor_data *d)
{
  const int columns = 3;
  int i, j;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif

  for (i = 0; i <= NUM_APPLY_TYPES / columns; ++i) {
    *buf = '\0';
    for (j = 0; j < columns; ++j)
      if (TYPE_INDEX < NUM_APPLY_TYPES)
        sprintf(buf, "%s%s%2d%s) %-20.20s ", buf,
                grn, TYPE_INDEX, nrm, apply_types[TYPE_INDEX]);
    send_to_char(strcat(buf, "\r\n"), d->character);
  }

  send_to_char("\r\nEnter apply type (0 is no apply) : ", d->character);
  OLC_MODE(d) = OEDIT_APPLY;
}
#undef TYPE_INDEX

/*
 * Weapon type.
 */
#define TYPE_INDEX    ((NUM_ATTACK_TYPES / columns + 1) * j + i)
void oedit_disp_weapon_menu(struct descriptor_data *d)
{
  const int columns = 3;
  int i, j;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif

  for (i = 0; i <= NUM_ATTACK_TYPES / columns; ++i) {
    *buf = '\0';
    for (j = 0; j < columns; ++j)
      if (TYPE_INDEX < NUM_ATTACK_TYPES)
        sprintf(buf, "%s%s%2d%s) %-20.20s ", buf,
                grn, TYPE_INDEX + 1, nrm, attack_hit_text[TYPE_INDEX].singular);
    send_to_char(strcat(buf, "\r\n"), d->character);
  }

  send_to_char("\r\nEnter weapon type : ", d->character);
}
#undef TYPE_INDEX

/*
 * Spell type.
 */
void oedit_disp_spells_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("^[[H^[[J", d->character);
#endif
  /* Fixed to use all spells --gurlaek 7/22/1999 */
  for (counter = 0; counter <= MAX_SPELLS; counter++) {
    if (strcmp(skills[counter].name, "!UNUSED!")) {
      sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
              skills[counter].name, !(++columns % 3) ? "\r\n" : "");
      send_to_char(buf, d->character);
    }
  }
  sprintf(buf, "\r\n%sEnter spell choice (0 for none) : ", nrm);
  send_to_char(buf, d->character);
}

void oedit_disp_portal_messages_menu(struct descriptor_data *d,
                                     const char *messages[]) {
  int i = 0;

  while (*messages[i] != '\n') {
    sprintf(buf, "%s%d%s) %s", grn, i, nrm, messages[i]);
    send_to_char(buf, d->character);
    ++i;
  }
}

void oedit_disp_boards(struct descriptor_data *d) {
  struct board_iter *iter = board_iterator();
  const struct board_data *board;
  int i = 1;

  while ((board = next_board(iter)))
    dprintf(d, "%s%d%s) %s\r\n", grn, i++, nrm, board->title);

  free_board_iterator(iter);

  dprintf(d, "\r\n%sEnter board choice: ", nrm);
}

/*
 * Object value #1
 */
void oedit_disp_val1_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_1;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_LIGHT:
    /* values 0 and 1 are unused.. jump to 2 */
    oedit_disp_val3_menu(d);
    break;
  case ITEM_SCROLL:
  case ITEM_WAND:
  case ITEM_STAFF:
  case ITEM_POTION:
    send_to_char("Spell level : ", d->character);
    break;
  case ITEM_WEAPON:
    /* this seems to be a circleism.. not part of normal diku? */
    send_to_char("Modifier to Hitroll : ", d->character);
    break;
  case ITEM_ARMOR:
    send_to_char("Apply to AC : ", d->character);
    break;
  case ITEM_CONTAINER:
    send_to_char("Max weight to contain : ", d->character);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    send_to_char("Capacity (ounces) : ", d->character);
    break;
  case ITEM_FOOD:
    send_to_char("Hours to fill stomach : ", d->character);
    break;
  case ITEM_SPELLBOOK:
    send_to_char("Pages in spellbook : ", d->character);
    break;
  case ITEM_MONEY:
    dprintf(d, "Number of %s coins : ", COIN_NAME(PLATINUM));
    break;
  case ITEM_PORTAL:
    send_to_char("Room to go to : ", d->character);
    break;
  case ITEM_WALL:
    oedit_disp_wall_block_dirs(d);
    break;
  case ITEM_BOARD:
    if (board_count())
      oedit_disp_boards(d);
    else
      dprintf(d, "No boards defined.  (Hit enter.)\r\n");
    break;
  default:
    limit_obj_values(OLC_OBJ(d));
    oedit_disp_menu(d);
  }
}

/*
 * Object value #2
 */
void oedit_disp_val2_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_2;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    send_to_char("Max number of charges : ", d->character);
    break;
  case ITEM_WEAPON:
    send_to_char("Number of damage dice : ", d->character);
    break;
  case ITEM_FOOD:
    /* values 2 and 3 are unused, jump to 4. how odd */
    oedit_disp_val4_menu(d);
    break;
  case ITEM_CONTAINER:
    /* these are flags, needs a bit of special handling */
    oedit_disp_container_flags_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    send_to_char("Initial contents (ounces): ", d->character);
    break;
  case ITEM_MONEY:
    dprintf(d, "Number of %s coins : ", COIN_NAME(GOLD));
    break;
  case ITEM_PORTAL:
    oedit_disp_portal_messages_menu(d, portal_entry_messages);
    send_to_char("Portal entry message to original room: ", d->character);
    break;
  case ITEM_WALL:
    send_to_char("Does wall crumble on dispel magic? (0)No (1)Yes : ",d->character);
    break;
  default:
    limit_obj_values(OLC_OBJ(d));
    oedit_disp_menu(d);
  }
}

/*
 * Object value #3
 */
void oedit_disp_val3_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_3;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_LIGHT:
    send_to_char("Number of hours (0 = burnt, -1 is infinite) : ", d->character);
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    send_to_char("Number of charges remaining : ", d->character);
    break;
  case ITEM_WEAPON:
    send_to_char("Size of damage dice : ", d->character);
    break;
  case ITEM_CONTAINER:
    send_to_char("Vnum of key to open container (-1 for no key) : ", d->character);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    oedit_liquid_type(d);
    break;
  case ITEM_MONEY:
    dprintf(d, "Number of %s coins : ", COIN_NAME(SILVER));
    break;
  case ITEM_PORTAL:
    oedit_disp_portal_messages_menu(d, portal_character_messages);
    send_to_char("Portal entry message to character: ", d->character);
    break;
  case ITEM_WALL:
    send_to_char("Wall Hit Points : ",d->character);
    break;
  default:
    limit_obj_values(OLC_OBJ(d));
    oedit_disp_menu(d);
  }
}

/*
 * Object value #4
 */
void oedit_disp_val4_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_4;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_SCROLL:
  case ITEM_POTION:
  case ITEM_WAND:
  case ITEM_STAFF:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WEAPON:
    oedit_disp_weapon_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
  case ITEM_FOOD:
    send_to_char("Poisoned (0 = not poison) : ", d->character);
    break;
  case ITEM_MONEY:
    dprintf(d, "Number of %s coins : ", COIN_NAME(COPPER));
    break;
  case ITEM_PORTAL:
    oedit_disp_portal_messages_menu(d, portal_exit_messages);
    send_to_char("Portal exit message to target room: ", d->character);
    break;
  default:
    limit_obj_values(OLC_OBJ(d));
    oedit_disp_menu(d);
  }
}

/*
 * Object type.
 */
#define FLAG_INDEX (((NUM_ITEM_TYPES - 1) / columns + 1) * j + i)
void oedit_disp_type_menu(struct descriptor_data *d)
{
  const int columns = 3;
  int i, j;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif

  for (i = 0; i <= (NUM_ITEM_TYPES - 1) / columns; ++i) {
    *buf = '\0';
    for (j = 0; j < columns; ++j)
      if (FLAG_INDEX < NUM_ITEM_TYPES - 1)
        sprintf(buf, "%s%s%2d%s) %-20.20s ", buf,
                grn, FLAG_INDEX + 1, nrm, item_types[FLAG_INDEX + 1].name);
    send_to_char(strcat(buf, "\r\n"), d->character);
  }
  send_to_char("\r\nEnter object type : ", d->character);
}
#undef FLAG_INDEX

/*
 * Object extra flags.
 */
#define FLAG_INDEX    ((NUM_ITEM_FLAGS / columns + 1) * j + i)
void oedit_disp_extra_menu(struct descriptor_data *d)
{
  const int columns = 3;
  int i, j;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif

  for (i = 0; i <= NUM_ITEM_FLAGS / columns; ++i) {
    *buf = '\0';
    for (j = 0; j < columns; ++j)
      if (FLAG_INDEX < NUM_ITEM_FLAGS)
        sprintf(buf, "%s%s%2d%s) %-20.20s ", buf,
                grn, FLAG_INDEX + 1, nrm, extra_bits[FLAG_INDEX]);
    send_to_char(strcat(buf, "\r\n"), d->character);
  }

  sprintflag(buf1, GET_OBJ_FLAGS(OLC_OBJ(d)), NUM_ITEM_FLAGS, extra_bits);
  sprintf(buf, "\r\nObject flags: %s%s%s\r\n"
      "Enter object extra flag (0 to quit) : ",
      cyn, buf1, nrm);
  send_to_char(buf, d->character);
}
#undef FLAG_INDEX

/*
 * Object wear flags.
 */
#define FLAG_INDEX    ((NUM_ITEM_WEAR_FLAGS / columns + 1) * j + i)
void oedit_disp_wear_menu(struct descriptor_data *d)
{
  const int columns = 3;
  int i, j;

  get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (i = 0; i <= NUM_ITEM_WEAR_FLAGS / columns; ++i) {
    *buf = '\0';
    for (j = 0; j < columns; ++j)
      if (FLAG_INDEX < NUM_ITEM_WEAR_FLAGS)
        sprintf(buf, "%s%s%2d%s) %-20.20s ", buf,
                grn, FLAG_INDEX + 1, nrm, wear_bits[FLAG_INDEX]);
    send_to_char(strcat(buf, "\r\n"), d->character);
  }

  sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, buf1);
  sprintf(buf, "\r\nWear flags: %s%s%s\r\n"
      "Enter wear flag, 0 to quit : ", cyn, buf1, nrm);
  send_to_char(buf, d->character);
}
#undef FLAG_INDEX


void oedit_disp_obj_values(struct descriptor_data *d) {
  struct obj_data *obj = OLC_OBJ(d);
  int i;

  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_LIGHT:
      if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
        sprintf(buf, "         Hours : %sInfinite%s\r\n", cyn, nrm);
      else
        sprintf(buf, "         Hours : %s%d%s\r\n",
                cyn, GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING), nrm);
      break;
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "   Spell Level : %s%d%s\r\n"
                   "        Spells : %s%s, %s, %s%s\r\n",
              cyn, GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL), nrm,
              cyn, skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1)),
              skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2)),
              skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3)), nrm);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "         Spell : %s%s%s\r\n"
                   "   Spell Level : %s%d%s\r\n"
                   "       Charges : %s%d remaining of %d max%s\r\n",
              cyn, skill_name(GET_OBJ_VAL(obj, VAL_WAND_SPELL)), nrm,
              cyn, GET_OBJ_VAL(obj, VAL_WAND_LEVEL), nrm,
              cyn, GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT),
                   GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES), nrm);
      break;
    case ITEM_WEAPON:
      sprintf(buf, "   Damage Dice : %s%dd%d%s\r\n"
                   "       Message : %s%s%s\r\n",
              cyn, GET_OBJ_VAL(obj, VAL_WEAPON_DICE_NUM),
                   GET_OBJ_VAL(obj, VAL_WEAPON_DICE_SIZE), nrm,
              cyn, GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) >= 0 &&
                   GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) <= TYPE_STAB - TYPE_HIT?
                   attack_hit_text[GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE)].singular :
                   "INVALID", nrm);
      break;
    case ITEM_ARMOR:
      sprintf(buf, "      AC-Apply : %s%d%s\r\n",
              cyn, GET_OBJ_VAL(obj, VAL_ARMOR_AC), nrm);
      break;
    case ITEM_TRAP:
      sprintf(buf, "         Spell : %s%s%s\r\n"
                   "     Hitpoints : %s%d%s\r\n",
              cyn, skill_name(GET_OBJ_VAL(obj, VAL_TRAP_SPELL)), nrm,
              cyn, GET_OBJ_VAL(obj, VAL_TRAP_HITPOINTS), nrm);
      break;
    case ITEM_CONTAINER:
      sprintf(buf, "      Capacity : %s%d%s\r\n",
              cyn, GET_OBJ_VAL(obj, VAL_CONTAINER_CAPACITY), nrm);
      break;
    case ITEM_NOTE:
      sprintf(buf, "        Tongue : %s%d%s\r\n",
              cyn, GET_OBJ_VAL(obj, 0), nrm);
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      sprintf(buf, "      Capacity : %s%d oz%s\r\n"
                   "      Contains : %s%d oz%s\r\n"
                   "      Poisoned : %s%s%s\r\n"
                   "        Liquid : %s%s%s\r\n",
              cyn, GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY), nrm,
              cyn, GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING), nrm,
              cyn, YESNO(IS_POISONED(obj)), nrm,
              cyn, LIQ_NAME(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)), nrm);
      break;
    case ITEM_FOOD:
      sprintf(buf, "    Makes full : %s%d hours%s\r\n"
                   "      Poisoned : %s%s%s\r\n",
              cyn, GET_OBJ_VAL(obj, VAL_FOOD_FILLINGNESS), nrm,
              cyn, YESNO(IS_POISONED(obj)), nrm);
      break;
    case ITEM_MONEY:
      sprintf(buf, "         Coins : %s%dp %dg %ds %dc%s\r\n",
              cyn, GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM),
                   GET_OBJ_VAL(obj, VAL_MONEY_GOLD),
                   GET_OBJ_VAL(obj, VAL_MONEY_SILVER),
                   GET_OBJ_VAL(obj, VAL_MONEY_COPPER), nrm);
      break;
    case ITEM_PORTAL:
      i = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DESTINATION));
      sprinttype(GET_OBJ_VAL(obj, VAL_PORTAL_ENTRY_MSG), portal_entry_messages, buf1);
      sprinttype(GET_OBJ_VAL(obj, VAL_PORTAL_CHAR_MSG), portal_character_messages, buf2);
      sprinttype(GET_OBJ_VAL(obj, VAL_PORTAL_EXIT_MSG), portal_exit_messages, arg);
      sprintf(buf, "   Target Room : %s%s (%d)%s\r\n"
                   " Entry Message : %s%s%s"
                   "  Char Message : %s%s%s"
                   "  Exit Message : %s%s%s",
              cyn, i == NOWHERE ? "Invalid Room" : world[i].name,
              GET_OBJ_VAL(obj, VAL_PORTAL_DESTINATION), nrm,
              cyn, buf1, nrm,
              cyn, buf2, nrm,
              cyn, arg, nrm);
      break;
    case ITEM_SPELLBOOK:
      sprintf(buf, "         Pages : %s%d%s\r\n",
              cyn, GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES), nrm);
      break;
    case ITEM_WALL:
      sprinttype(GET_OBJ_VAL(obj, VAL_WALL_DIRECTION), dirs, buf1);
      sprintf(buf, "     Direction : %s%s%s\r\n"
                   "    Dispelable : %s%s%s\r\n"
                   "     Hitpoints : %s%d%s\r\n",
              cyn, buf1, nrm,
              cyn, YESNO(GET_OBJ_VAL(obj, VAL_WALL_DISPELABLE)), nrm,
              cyn, GET_OBJ_VAL(obj, VAL_WALL_HITPOINTS), nrm);
      break;
    case ITEM_BOARD:
      sprintf(buf, "   Board Title : %s%s%s\r\n",
              cyn, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER))->title, nrm);
      break;
    default:
      return;
  }
  send_to_char(buf, d->character);
}


/*
 * Display main menu.
 */
void oedit_disp_menu(struct descriptor_data *d)
{
  struct obj_data *obj;

  obj = OLC_OBJ(d);
  get_char_cols(d->character);

  /*. Build buffers for first part of menu .*/
  sprintflag(buf2, GET_OBJ_FLAGS(obj), NUM_ITEM_FLAGS, extra_bits);

  /*
   * Build first half of menu.
   */
  sprintf(buf,
#if defined(CLEAR_SCREEN)
      ".[H.[J"
#endif
      "-- Item: '&5%s&0'  vnum: [&2%5d&0]\r\n"
      "%s1%s) Namelist : %s%s\r\n"
      "%s2%s) S-Desc   : %s%s\r\n"
      "%s3%s) L-Desc   :-\r\n%s%s\r\n"
      "%s4%s) A-Desc   :-\r\n%s%s"
      "%s5%s) Type        : %s%s\r\n"
      "%s6%s) Extra flags : %s%s\r\n",


      (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
      OLC_NUM(d),
      grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined",
      grn, nrm, yel, (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
      grn, nrm, yel, (obj->description && *obj->description) ? obj->description : "undefined",
      grn, nrm, yel, (obj->action_description && *obj->action_description) ? obj->action_description : "<not set>\r\n",
      grn, nrm, cyn, OBJ_TYPE_NAME(obj),
      grn, nrm, cyn, buf2
      );
  /*
   * Send first half.
   */
  send_to_char(buf, d->character);

  /*. Build second half of menu .*/
  sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf1);
  sprintf(buf,
      "%s7%s) Wear flags  : %s%s\r\n"
      "%s8%s) Weight      : %s%.2f\r\n"
      "%s9%s) Cost        : %s%d\r\n"
      "%sA%s) Timer       : %s%d\r\n"
      "%sB%s) Level       : %s%d\r\n"
      "%sC%s) Hiddenness  : %s%ld\r\n"
      "%sD%s) Values      : %s%d %d %d %d %d %d %d%s\r\n",

      grn, nrm, cyn, buf1,
      grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
      grn, nrm, cyn, GET_OBJ_COST(obj),
      grn, nrm, cyn, GET_OBJ_TIMER(obj),
      grn, nrm, cyn, GET_OBJ_LEVEL(obj),
          grn, nrm, cyn, GET_OBJ_HIDDENNESS(obj),
      grn, nrm, cyn, GET_OBJ_VAL(obj, 0),
      GET_OBJ_VAL(obj, 1),
      GET_OBJ_VAL(obj, 2),
      GET_OBJ_VAL(obj, 3),
      GET_OBJ_VAL(obj, 4),
      GET_OBJ_VAL(obj, 5),
      GET_OBJ_VAL(obj, 6),
      nrm
      );

  send_to_char(buf, d->character);

  oedit_disp_obj_values(d);

  *buf1 = '\0';
  sprintflag(buf1, GET_OBJ_EFF_FLAGS(obj), NUM_EFF_FLAGS, effect_flags);
  sprintf(buf,
          "%sE%s) Applies menu\r\n"
      "%sF%s) Extra descriptions menu\r\n"
      "%sG%s) Spell applies : &6%s&0\r\n"
      "%sS%s) Script      : %s%s\r\n"
      "%sQ%s) Quit\r\n"
      "Enter choice : ",

      grn, nrm,
      grn, nrm,
      grn, nrm, buf1,
      grn, nrm,
      cyn, obj->proto_script?"Set.":"Not Set.",
      grn, nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 *  main loop (of sorts).. basically interpreter throws all input to here  *
 ***************************************************************************/


void oedit_parse(struct descriptor_data *d, char *arg)
{
  int number = atoi(arg);
  float fnum = atof(arg);

  switch (OLC_MODE(d)) {

  case OEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      if (STATE(d) == CON_IEDIT) {
        write_to_output("Saving changes to object.\r\n", d);
        iedit_save_changes(d);
        sprintf(buf, "OLC: %s edits unique obj %s", GET_NAME(d->character), OLC_IOBJ(d)->short_description);
        mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
        REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
        STATE(d) = CON_PLAYING;
      }
      else {
        send_to_char("Saving object to memory.\r\n", d->character);
        oedit_save_internally(d);
        sprintf(buf, "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
        mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
      }
      cleanup_olc(d, CLEANUP_STRUCTS);
      return;
    case 'n':
    case 'N':
      /*. Cleanup all .*/
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save this object internally?\r\n", d->character);
      return;
    }

  case OEDIT_MAIN_MENU:
    /* throw us out to whichever edit mode based on user input */
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) {    /* Something has been modified. */
        send_to_char("Do you wish to save this object internally? : ", d->character);
        OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
      } else
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      send_to_char("Enter namelist : ", d->character);
      OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
      break;
    case '2':
      send_to_char("Enter short desc : ", d->character);
      OLC_MODE(d) = OEDIT_SHORTDESC;
      break;
    case '3':
      send_to_char("Enter long desc :-\r\n| ", d->character);
      OLC_MODE(d) = OEDIT_LONGDESC;
      break;
    case '4':
      OLC_MODE(d) = OEDIT_ACTDESC;
      write_to_output("Enter action description: (/s saves /h for help)\r\n\r\n", d);
      string_write(d, &OLC_OBJ(d)->action_description, MAX_DESC_LENGTH);
      OLC_VAL(d) = 1;
      break;
    case '5':
      oedit_disp_type_menu(d);
      OLC_MODE(d) = OEDIT_TYPE;
      break;
    case '6':
      oedit_disp_extra_menu(d);
      OLC_MODE(d) = OEDIT_EXTRAS;
      break;
    case '7':
      oedit_disp_wear_menu(d);
      OLC_MODE(d) = OEDIT_WEAR;
      break;
    case '8':
      send_to_char("Enter weight : ", d->character);
      OLC_MODE(d) = OEDIT_WEIGHT;
      break;
   case '9':
      send_to_char("Enter cost (copper) : ", d->character);
      OLC_MODE(d) = OEDIT_COST;
      break;
    case 'a':
    case 'A':
      send_to_char("Enter timer : ", d->character);
      OLC_MODE(d) = OEDIT_TIMER;
      break;
    case 'b':
    case 'B':
      send_to_char("Enter level : ", d->character);
      OLC_MODE(d) = OEDIT_LEVEL;
      break;
    case 'c':
    case 'C':
      send_to_char("Enter hiddenness : ", d->character);
      OLC_MODE(d) = OEDIT_HIDDENNESS;
      break;
    case 'd':
    case 'D':
      /*
       * Clear any old values
       */
      for (number = 0; number < NUM_VALUES; ++number)
        GET_OBJ_VAL(OLC_OBJ(d), number) = 0;
      oedit_disp_val1_menu(d);
      break;
    case 'e':
    case 'E':
      oedit_disp_prompt_apply_menu(d);
      break;
    case 'f':
    case 'F':
      /*
       * If extra descriptions don't exist.
       */
      if (!OLC_OBJ(d)->ex_description) {
    CREATE(OLC_OBJ(d)->ex_description, struct extra_descr_data, 1);
    OLC_OBJ(d)->ex_description->next = NULL;
      }
      OLC_DESC(d) = OLC_OBJ(d)->ex_description;
      oedit_disp_extradesc_menu(d);
      break;
    case 'g':
    case 'G':
      oedit_disp_aff_flags(d);
      OLC_MODE(d) = OEDIT_SPELL_APPLY;
      break;
/*
    case 'h':
    case 'H':
      OLC_MODE(d) = OEDIT_SPELL_COMPONENT;
      oedit_disp_component(d);
      break;
*/
    case 'p':
    case 'P':
      if (GET_OBJ_RNUM(OLC_OBJ(d)) == NOTHING)
      {
        send_to_char("You cannot purge a non-existent (unsaved) objext! Choose again:\r\n",d->character);
      }
      else if (GET_LEVEL(d->character)<LVL_HEAD_B)
      {
        sprintf(buf,"You are too low level to purge! Get a level %d or greater to do this.\r\n",LVL_HEAD_B);
        send_to_char(buf,d->character);
      }
      else
      {
        OLC_MODE(d) = OEDIT_PURGE_OBJECT;
        /* make extra sure*/
        send_to_char("Purging will also remove all existing objects of this sort!\r\n", d->character);
        send_to_char("Are you sure you wish to PERMANENTLY DELETE the object? (y/n) : ", d->character);
      }
      return;
    case 's':
    case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;


    default:
      oedit_disp_menu(d);
      break;
    }
    return;            /* end of OEDIT_MAIN_MENU */


  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg)) return;
    break;
  case OEDIT_EDIT_NAMELIST:
    if (OLC_OBJ(d)->name)
      free(OLC_OBJ(d)->name);
    OLC_OBJ(d)->name = strdup((arg && *arg) ? arg : "undefined");
    break;

  case OEDIT_SHORTDESC:
    if (OLC_OBJ(d)->short_description)
      free(OLC_OBJ(d)->short_description);
    OLC_OBJ(d)->short_description = strdup((arg && *arg) ? arg : "undefined");
    break;

  case OEDIT_LONGDESC:
    if (OLC_OBJ(d)->description)
      free(OLC_OBJ(d)->description);
    OLC_OBJ(d)->description = strdup((arg && *arg) ? arg : "undefined");
    break;

  case OEDIT_TYPE:
    if ((number < 1) || (number > NUM_ITEM_TYPES)) {
      send_to_char("Invalid choice, try again : ", d->character);
      return;
    }
    GET_OBJ_TYPE(OLC_OBJ(d)) = number;
    limit_obj_values(OLC_OBJ(d));
    break;
  case OEDIT_EXTRAS:
    if ((number < 0) || (number > NUM_ITEM_FLAGS)) {
      oedit_disp_extra_menu(d);
      return;
    } else if (number == 0)
      break;
    else {
      TOGGLE_FLAG(GET_OBJ_FLAGS(OLC_OBJ(d)), number - 1);
      /* This flag shouldn't be on object prototypes */
      REMOVE_FLAG(GET_OBJ_FLAGS(OLC_OBJ(d)), ITEM_WAS_DISARMED);
      oedit_disp_extra_menu(d);
      return;
    }

  case OEDIT_WEAR:
    if ((number < 0) || (number > NUM_ITEM_WEAR_FLAGS)) {
      send_to_char("That's not a valid choice!\r\n", d->character);
      oedit_disp_wear_menu(d);
      return;
    } else if (number == 0)    /* Quit. */
      break;
    else {
      TOGGLE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (number - 1));
      oedit_disp_wear_menu(d);
      return;
    }

  case OEDIT_WEIGHT:
    GET_OBJ_WEIGHT(OLC_OBJ(d)) = fnum;
    break;

  case OEDIT_COST:
    GET_OBJ_COST(OLC_OBJ(d)) = number;
    break;

  case OEDIT_TIMER:
    GET_OBJ_TIMER(OLC_OBJ(d)) = number;
    break;

  case OEDIT_LEVEL:
    GET_OBJ_LEVEL(OLC_OBJ(d)) = number;
    break;

  case OEDIT_HIDDENNESS:
    GET_OBJ_HIDDENNESS(OLC_OBJ(d)) = LIMIT(0, number, 1000);
    break;

  case OEDIT_VALUE_1:
    /* Range-check values at the very end by calling limit_obj_values */
    GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
    OLC_VAL(d) = 1;
    /* Proceed to menu 2. */
    oedit_disp_val2_menu(d);
    return;

  case OEDIT_VALUE_2:
    /* Check for out of range values. */
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
      if (number < 0 || number > MAX_SPELLS ||
          !strcmp(skills[number].name, "!UNUSED!")) {
        oedit_disp_val2_menu(d);
        return;
      }
      break;
    case ITEM_CONTAINER:
      if (number < 0 || number > 4)
        oedit_disp_container_flags_menu(d);
      else if (number != 0) {
        TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), VAL_CONTAINER_BITS), 1 << (number - 1));
        oedit_disp_val2_menu(d);
      } else
        oedit_disp_val3_menu(d);
      return;
    default:
      GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
    }
    OLC_VAL(d) = 1;
    oedit_disp_val3_menu(d);
    return;

  case OEDIT_VALUE_3:
    /*
     * Quick'n'easy error checking.
     */
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      GET_OBJ_VAL(OLC_OBJ(d), 2) = number;
      if (number < 0 || number > MAX_SPELLS ||
          !strcmp(skills[number].name, "!UNUSED!")) {
        oedit_disp_val3_menu(d);
        return;
      }
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      --number; /* Types are displayed starting with 1 index */
      /* fall through to default! */
    default:
      GET_OBJ_VAL(OLC_OBJ(d), 2) = number;
    }
    OLC_VAL(d) = 1;
    oedit_disp_val4_menu(d);
    return;

  case OEDIT_VALUE_4:
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_WAND:
    case ITEM_STAFF:
      GET_OBJ_VAL(OLC_OBJ(d), 3) = number;
      if (number < 0 || number > MAX_SPELLS ||
          !strcmp(skills[number].name, "!UNUSED!")) {
        oedit_disp_val4_menu(d);
        return;
      }
      break;
    case ITEM_WEAPON:
      --number; /* Types are displayed starting with 1 index */
      /* fall through to default! */
    default:
      GET_OBJ_VAL(OLC_OBJ(d), 3) = number;
    }
    OLC_VAL(d) = 1;    
    limit_obj_values(OLC_OBJ(d));
    break;

  case OEDIT_PROMPT_APPLY:
    if (number == 0)
      break;
    else if (number < 0 || number > MAX_OBJ_APPLIES) {
      oedit_disp_prompt_apply_menu(d);
      return;
    }
    OLC_VAL(d) = number - 1;
    OLC_MODE(d) = OEDIT_APPLY;
    oedit_disp_apply_menu(d);
    return;

  case OEDIT_APPLY:
    if (number == 8) {
      send_to_char("I don't think so\r\n", d->character);
      return;
    }
    if (number == 0) {
      OLC_OBJ(d)->applies[OLC_VAL(d)].location = 0;
      OLC_OBJ(d)->applies[OLC_VAL(d)].modifier = 0;
      oedit_disp_prompt_apply_menu(d);
    } else if (number < 0 || number >= NUM_APPLY_TYPES)
      oedit_disp_apply_menu(d);
    else {
      OLC_OBJ(d)->applies[OLC_VAL(d)].location = number;
      if (number == APPLY_COMPOSITION) {
        list_olc_compositions(d->character);
        send_to_char("Composition : ", d->character);
      } else
        send_to_char("Modifier : ", d->character);
      OLC_MODE(d) = OEDIT_APPLYMOD;
    }
    return;

  case OEDIT_APPLYMOD:
    if (OLC_OBJ(d)->applies[OLC_VAL(d)].location == APPLY_COMPOSITION &&
        (number < 0 || number >= NUM_COMPOSITIONS)) {
      send_to_char("Invalid composition!\r\n", d->character);
      list_olc_compositions(d->character);
      send_to_char("Composition : ", d->character);
      return;
    }
    OLC_OBJ(d)->applies[OLC_VAL(d)].modifier = number;
    oedit_disp_prompt_apply_menu(d);
    return;

  case OEDIT_SPELL_APPLY:
    if (number == 0)
      break;
    else if (number > 0 && number <= NUM_EFF_FLAGS)
      TOGGLE_FLAG(GET_OBJ_EFF_FLAGS(OLC_OBJ(d)), number - 1);
    else
      send_to_char("That's not a valid choice!\r\n", d->character);
    oedit_disp_aff_flags(d);
    return;

  case OEDIT_EXTRADESC_KEY:
    if (OLC_DESC(d)->keyword)
      free(OLC_DESC(d)->keyword);
    OLC_DESC(d)->keyword = strdup((arg && *arg) ? arg : "undefined");
    oedit_disp_extradesc_menu(d);
    return;

  case OEDIT_EXTRADESC_MENU:
    switch (number) {
    case 0:
      if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
    struct extra_descr_data **tmp_desc;

    if (OLC_DESC(d)->keyword)
      free(OLC_DESC(d)->keyword);
    if (OLC_DESC(d)->description)
      free(OLC_DESC(d)->description);

    /*
     * Clean up pointers
     */
    for (tmp_desc = &(OLC_OBJ(d)->ex_description); *tmp_desc;
         tmp_desc = &((*tmp_desc)->next)) {
      if (*tmp_desc == OLC_DESC(d)) {
        *tmp_desc = NULL;
        break;
      }
    }
    free(OLC_DESC(d));
      }
      break;

    case 1:
      OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
      send_to_char("Enter keywords, separated by spaces :-\r\n| ", d->character);
      return;

    case 2:
      OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
      write_to_output("Enter the extra description: (/s saves /h for help)\r\n\r\n", d);
      string_write(d, &OLC_DESC(d)->description, MAX_DESC_LENGTH);
      OLC_VAL(d) = 1;
      return;

    case 3:
      /*
       * Only go to the next description if this one is finished.
       */
      if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) {
    struct extra_descr_data *new_extra;
    if (OLC_DESC(d)->next)
      OLC_DESC(d) = OLC_DESC(d)->next;
    else {    /* Make new extra description and attach at end. */
      CREATE(new_extra, struct extra_descr_data, 1);

      OLC_DESC(d)->next = new_extra;
      OLC_DESC(d) = OLC_DESC(d)->next;
    }
      }
      /*. No break - drop into default case .*/
    default:
      oedit_disp_extradesc_menu(d);
      return;
    }
    break;
    case OEDIT_PURGE_OBJECT:
      switch (*arg) {
      case 'y':
      case 'Y':
    /*. Splat the object in memory ..*/
    send_to_char("Purging object from memory.\r\n", d->character);

    /*need to remove all existing objects of this type too..*/
    /*ok..we use save internally, but we are purging because of the mode*/
    oedit_save_internally(d);
    sprintf(buf, "OLC: %s PURGES object %d", GET_NAME(d->character), OLC_NUM(d));
    mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
    /* FALL THROUGH */
      case 'n':
      case 'N':
    cleanup_olc(d, CLEANUP_ALL);
    return;
      default:
    send_to_char("Invalid choice!\r\n", d->character);
    send_to_char("Do you wish to purge the object? : ", d->character);
    return;
      }
      break;
  default:
    mudlog("SYSERR: OLC: Reached default case in oedit_parse()!", BRF, LVL_GOD, TRUE);
    send_to_char("Oops...\r\n", d->character);
    break;
  }

  /*
   * If we get here, we have changed something.
   */
  OLC_VAL(d) = 1;
  oedit_disp_menu(d);
}


void iedit_setup_existing(struct descriptor_data *d, struct obj_data *obj)
{
  struct obj_data *temp;

  /* So there's no way for this obj to get extracted (by point_update 
   * for example) */
  REMOVE_FROM_LIST(obj, object_list, next);

  /* free any assigned scripts */
  if (SCRIPT(obj))
    extract_script(SCRIPT(obj));
  SCRIPT(obj) = NULL;

  CREATE(OLC_OBJ(d), struct obj_data, 1);

  copy_object(OLC_OBJ(d), obj);

  OLC_IOBJ(d) = obj; /* save reference to real object */
  OLC_VAL(d) = 0;
  OLC_NUM(d) = NOTHING;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  dg_olc_script_copy(d);
  OLC_OBJ(d)->proto_script = NULL;
  oedit_disp_menu(d);
}

void iedit_save_changes(struct descriptor_data *d)
{
  struct obj_data *obj = OLC_IOBJ(d);

  GET_ID(OLC_OBJ(d)) = GET_ID(obj);
  if (GET_OBJ_RNUM(obj) != NOTHING)
    obj_index[GET_OBJ_RNUM(obj)].number--;
  copy_object(obj, OLC_OBJ(d));
  obj->proto_script = OLC_SCRIPT(d);
  GET_OBJ_RNUM(obj) = NOTHING;
}       


ACMD(do_iedit)
{
  struct obj_data *obj;
  int i;

  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Which object do you want to edit?\r\n", ch);
    return;
  }

  if ((obj = find_obj_in_eq(ch, &i, find_vis_by_name(ch, arg))))
    unequip_char(ch, i);
  else if ((obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg))))
    obj_from_char(obj);
  else if ((obj = find_obj_in_list(world[IN_ROOM(ch)].contents, find_vis_by_name(ch, arg))))
    obj_from_room(obj);
  else {
    send_to_char("Object not found.\r\n", ch);
    return;
  }

  /* Setup OLC */
  CREATE(ch->desc->olc, struct olc_data, 1);

  SET_FLAG(PLR_FLAGS(ch), PLR_WRITING);
  iedit_setup_existing(ch->desc, obj);

  act("$n starts using OLC.", TRUE, ch, 0, 0, TO_ROOM);

  STATE(ch->desc) = CON_IEDIT;  
}


/***************************************************************************
 * $Log: oedit.c,v $
 * Revision 1.58  2009/03/20 20:19:51  myc
 * Removing dependency upon old board system.
 *
 * Revision 1.57  2009/03/20 13:56:22  jps
 * Moved coin info into an array of struct coindef.
 *
 * Revision 1.56  2009/03/09 21:43:50  myc
 * Use references to coin_names instead of string constants.
 *
 * Revision 1.55  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.54  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.53  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.52  2009/02/21 03:30:16  myc
 * Added new board type.
 *
 * Revision 1.51  2008/09/29 03:24:44  jps
 * Make container weight automatic. Move some liquid container functions to objects.c.
 *
 * Revision 1.50  2008/09/22 02:09:17  jps
 * Changed weight into a floating-point value. Precision is preserved to
 * the 1/100 place.
 *
 * Revision 1.49  2008/09/03 17:34:08  myc
 * Moved liquid information into a def struct array.
 *
 * Revision 1.48  2008/09/03 07:14:34  myc
 * Prevent point_update from extracting an object being iedited.
 *
 * Revision 1.47  2008/08/30 18:20:53  myc
 * Changed some rnum checks to compare against NOTHING constant.
 * Removed UNIQUE item flag.  Fixed obj index number corruption
 * bug.
 *
 * Revision 1.46  2008/08/29 05:34:24  myc
 * Fix doubled 'X stops using OLC.' message when exiting iedit.
 *
 * Revision 1.45  2008/08/29 05:26:06  myc
 * Make sure object prototypes don't have the UNIQUE or WAS_DISARMED flags.
 *
 * Revision 1.44  2008/08/29 05:14:02  myc
 * Removed an extra free_object_strings_proto call that was causing
 * crashes due to double frees.
 *
 * Revision 1.43  2008/08/26 03:58:13  jps
 * Replaced real_zone calls with find_real_zone_by_room, since that's what it did.
 * Except the one for wzoneecho, since it needed to find a real zone by zone number.
 *
 * Revision 1.42  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.41  2008/07/22 07:25:26  myc
 * Added basic iedit (unique item editor) functionality.
 *
 * Revision 1.40  2008/07/18 16:36:22  jps
 * Revert last change - it wasn't trivial.
 *
 * Revision 1.39  2008/07/14 15:50:51  jps
 * Don't update in-game objects when saving a modified object prototype.
 *
 * Revision 1.38  2008/06/19 19:12:54  myc
 * Count an object as being modified when having changed an item value.
 *
 * Revision 1.37  2008/06/19 19:08:17  myc
 * Show pages in spellbook in oedit menu.
 *
 * Revision 1.36  2008/06/19 18:53:12  myc
 * Expaneded item values to 7.  Replaced the item_types and item_type_desc
 * arrays with a typedef struct array that also describes the min and max
 * values for a particular item type.  Oedit now uses these values instead
 * of the ones previously hard-coded into the parser.
 *
 * Revision 1.35  2008/06/11 23:05:02  jps
 * Changed the intro line of the oedit menu.
 *
 * Revision 1.34  2008/06/07 19:06:46  myc
 * Moved object-related constants and routines to objects.h.
 *
 * Revision 1.33  2008/06/05 02:07:43  myc
 * Removed several fields from the object structure, including
 * cost_per_day, spare1, spare2, spare3, and spell component.
 * Changed object flags to use flagvectors.  Fixed an alignment
 * bug in the type menu.
 *
 * Revision 1.32  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.31  2008/04/20 17:48:26  jps
 * Removing unneeded externs.
 *
 * Revision 1.30  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.29  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.28  2008/03/23 00:25:41  jps
 * Allow editing of the composition apply.
 *
 * Revision 1.27  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.26  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.25  2008/03/17 16:22:42  myc
 * Fixed handling of proto scripts in OLC, including the squashing of
 * a memory leak.  Also fixed a possible premature freeing of memory.
 *
 * Revision 1.24  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.23  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.22  2008/01/29 16:51:12  myc
 * Moving skill names to the skilldef struct.
 *
 * Revision 1.21  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.20  2007/11/18 16:51:55  myc
 * Fixing LVL_BUILDER references.
 *
 * Revision 1.19  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  Hiddenness can be set for objects
 * in oedit.  Cleaned up object value editing and display in oedit.
 * Many values are now checked against min/max values.
 *
 * Revision 1.18  2007/09/15 15:36:48  myc
 * Was zeroing the wrong buffer.
 *
 * Revision 1.17  2007/09/15 05:03:46  myc
 * Implemented a new loop method for some of the menus so that items in
 * the menus get listed column-major instead of by rows.  This applies
 * to the spell applies, liquid types, apply types, weapon types, item
 * types, extra flags, and wear types menus.  Removed a dangerous (small)
 * buffer from oedit_disp_menu.  Removed the in-game distinction between
 * aff 1, 2, and 3 flags.  All aff flags are created equal now, at least
 * from the builder's perspective.
 *
 * Revision 1.16  2007/07/24 23:02:52  jps
 * Minor typo fix.
 *
 * Revision 1.15  2007/07/18 23:01:52  jps
 * Split "oedit revex" into "oedit zrevex" for an entire zone and
 * "oedit revex" for a single object.
 *
 * Revision 1.14  2007/07/18 22:28:47  jps
 * Added syntax "oedit revex <zone>" to reverse extra descs that may
 * have ended up reversed due to the way db.c used to read them.
 *
 * Revision 1.13  2007/07/18 01:21:34  jps
 * You can edit AFF2/AFF3 flags with oedit.
 *
 * Revision 1.12  2007/07/15 21:16:12  myc
 * No more crash when you edit a script on a new object or room.
 *
 * Revision 1.11  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.10  2002/07/14 03:41:18  rls
 * removed purge functionality from menu
 *
 * Revision 1.9  2001/07/08 16:01:22  mtp
 * added safety check for purge of level LVL_HEAD_B (currently 103)
 *
 * Revision 1.7  2001/03/24 05:12:01  dce
 * Objects will now accept a level through olc and upon
 * booting the objects. The level code for the players will
 * follow.
 *
 * Revision 1.6  2000/11/24 19:24:58  rsd
 * Altered comment header and added back rlog messages from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.5  2000/10/14 11:12:40  mtp
 * fixed the olc triggers editting in medit/oedit/redit
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/07/22 17:43:59  jimmy
 * removed the #define NUM_SPELLS that was wrong and used MAX_SPELLS instead.
 * Now all spells are available to assign to scrolls, objs etc.  Not
 * just 71.  This was done to reimplement the IDENTIFY scroll.
 * --gurlaek
 *
 * Revision 1.2  1999/01/31 22:05:32  mud
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/
