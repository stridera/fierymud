#pragma once

/* structure for the reset commands */
struct ResetCommand {
    char command; /* current command                      */

    bool if_flag; /* if true: exe only if preceding exe'd */
    int arg1;     /*                                      */
    int arg2;     /* Arguments to the command             */
    int arg3;     /*                                      */
    /*int  arg4;*/
    char *sarg; /*string command oneday be alot*/
    int line;   /* line number this command appears on  */

    /*
     *  Commands:              *
     *  'M': Read a mobile     *
     *  'O': Read an object    *
     *  'G': Give obj to mob   *
     *  'P': Put obj in obj    *
     *  'G': Obj to char       *
     *  'E': Obj to char equip *
     *  'D': Set state of door *
     */
};

/* zone definition structure. for the 'zone-table'   */
struct ZoneData {
    char *name;      /* name of this zone                  */
    int lifespan;    /* how long between resets (minutes)  */
    int age;         /* current age of this zone (minutes) */
    int top;         /* upper limit for rooms in this zone */
    int zone_factor; /* Unused. */
    int reset_mode;  /* conditions for reset (see below)   */
    int number;      /* vnum of this zone	  */

    /* weather information */
    int hemisphere;
    int temperature;
    int precipitation;
    int climate;
    int wind_speed;
    int wind_dir;

    /* if disaster_type = 0, no disaster in effect.  Otherwise, the
     * value is the disaster type in effect
     */
    int disaster_type;
    int disaster_duration;

    ResetCommand *cmd; /* command table for reset	          */

    /*
     *  Reset mode:                              *
     *  0: Don't reset, and don't update age.    *
     *  1: Reset if no PC's are located in zone. *
     *  2: Just reset.                           *
     */
};

/* for queueing zones for update   */
struct ResetQElement {
    int zone_to_reset; /* ref to zone_data */
    ResetQElement *next;
};

/* structure for the update queue     */
struct ResetQType {
    ResetQElement *head;
    ResetQElement *tail;
};
