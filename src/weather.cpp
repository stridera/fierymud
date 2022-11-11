/***************************************************************************
 *   File: weather.c                                      Part of FieryMUD *
 *  Usage: Weather functions, and structure initialization                 *
 *     By: Tim Holcomb (Fingh on Hubis)                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "weather.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "math.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/*
 * Initialize hemisphere data.  Only the names are permanent.  The
 * sunlight and season values are re-initialized from time data when
 * the game boots (and are updated as the game runs).
 */

void increment_game_time(void) {
    time_info.hours++;

    if (time_info.hours >= HOURS_PER_DAY) {
        time_info.hours = 0;
        ++time_info.day;
        if (time_info.day >= DAYS_PER_MONTH) {
            time_info.day = 0;
            ++time_info.month;
            if (time_info.month >= MONTHS_PER_YEAR) {
                time_info.month = 0;
                ++time_info.year;
            }
            update_season();
        }
    }
    update_daylight();
}

void update_daylight() {
    DescriptorData *d;

    switch (time_info.hours) {
    case 6:
        hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_SET;
        hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_RISE;
        hemispheres[HEMISPHERE_SOUTHEAST].sunlight = SUN_SET;
        hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_RISE;
        break;
    case 7:
        hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_DARK;
        hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_LIGHT;
        hemispheres[HEMISPHERE_SOUTHEAST].sunlight = SUN_DARK;
        hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_LIGHT;
        break;
    case 19:
        hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_RISE;
        hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_SET;
        hemispheres[HEMISPHERE_SOUTHEAST].sunlight = SUN_RISE;
        hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_SET;
        break;
    case 20:
        hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_LIGHT;
        hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_DARK;
        hemispheres[HEMISPHERE_SOUTHEAST].sunlight = SUN_LIGHT;
        hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_DARK;
        break;
    default:
        /* Don't send daylight change messages. */
        return;
    }

    /* Season change: send the message to everybody. */
    for (d = descriptor_list; d; d = d->next)
        if (STATE(d) == CON_PLAYING && d->character && AWAKE(d->character) && CH_OUTSIDE(d->character))
            write_to_output(daylight_change[hemispheres[IN_HEMISPHERE(d->character)].sunlight], d);
}

void update_season() {
    DescriptorData *d;

    switch (time_info.month) {
    case 0:
        hemispheres[HEMISPHERE_NORTHEAST].season = WINTER;
        hemispheres[HEMISPHERE_NORTHWEST].season = WINTER;
        hemispheres[HEMISPHERE_SOUTHEAST].season = SUMMER;
        hemispheres[HEMISPHERE_SOUTHWEST].season = SUMMER;
        break;
    case 4:
        hemispheres[HEMISPHERE_NORTHEAST].season = SPRING;
        hemispheres[HEMISPHERE_NORTHWEST].season = SPRING;
        hemispheres[HEMISPHERE_SOUTHEAST].season = AUTUMN;
        hemispheres[HEMISPHERE_SOUTHWEST].season = AUTUMN;
        break;
    case 8:
        hemispheres[HEMISPHERE_NORTHEAST].season = SUMMER;
        hemispheres[HEMISPHERE_NORTHWEST].season = SUMMER;
        hemispheres[HEMISPHERE_SOUTHEAST].season = WINTER;
        hemispheres[HEMISPHERE_SOUTHWEST].season = WINTER;
        break;
    case 12:
        hemispheres[HEMISPHERE_NORTHEAST].season = AUTUMN;
        hemispheres[HEMISPHERE_NORTHWEST].season = AUTUMN;
        hemispheres[HEMISPHERE_SOUTHEAST].season = SPRING;
        hemispheres[HEMISPHERE_SOUTHWEST].season = SPRING;
        break;
    default:
        /* Don't send new season messages. */
        return;
    }

    /* Season change: send the message to everybody. */
    for (d = descriptor_list; d; d = d->next)
        if (STATE(d) == CON_PLAYING && d->character && AWAKE(d->character) && CH_OUTSIDE(d->character))
            write_to_output(season_change[hemispheres[IN_HEMISPHERE(d->character)].season], d);

    /* Refresh weather for all zones for new season. */
    init_weather();
}

static CBP_FUNC(cb_outdoor) {
    CharData *ch = (CharData *)obj;
    int zone_rnum = (intptr_t)data;

    return (AWAKE(ch) && CH_OUTSIDE(ch) && IN_ZONE_RNUM(ch) == zone_rnum);
}

void init_weather() {
    int i;
    int climate;

    /* Initialize weather for each zone based on its climate. */
    for (i = 0; i < top_of_zone_table; ++i) {
        climate = zone_table[i].climate;

        zone_table[i].temperature = climates[climate].base_temperature;
        zone_table[i].precipitation = climates[climate].base_precipitation_rate;
        zone_table[i].wind_speed = climates[climate].base_wind_speed;
        zone_table[i].wind_dir = number(0, 3); /* Cardinal directions */
        zone_table[i].disaster_type = DISASTER_NONE;
        zone_table[i].disaster_duration = 0;

        switch (hemispheres[zone_table[i].hemisphere].season) {
        case WINTER:
            zone_table[i].temperature += WINTER_TEMP;
            zone_table[i].precipitation += WINTER_PREC;
            break;
        case SPRING:
            zone_table[i].temperature += SPRING_TEMP;
            zone_table[i].precipitation += SPRING_PREC;
            break;
        case SUMMER:
            zone_table[i].temperature += SUMMER_TEMP;
            zone_table[i].precipitation += SUMMER_PREC;
            break;
        case AUTUMN:
            zone_table[i].temperature += AUTUMN_TEMP;
            zone_table[i].precipitation += AUTUMN_PREC;
            break;
        }

        /* Check values for correctness. */
        zone_table[i].temperature = LIMIT(TEMP_FREEZING, zone_table[i].temperature, TEMP_FIRE_PLANE);
        zone_table[i].precipitation = LIMIT(PRECIP_NONE, zone_table[i].precipitation, PRECIP_DANGEROUS);
        zone_table[i].wind_speed = LIMIT(WIND_NONE, zone_table[i].wind_speed, WIND_HURRICANE);
    }
}

char *wind_message(int current, int original) {
    if (original == WIND_NONE) {
        if (current == WIND_NONE)
            strcpy(buf, "&6The air is calm.&0\r\n");
        else
            sprintf(buf, "&6A &0%s&0 &6begins to blow around you.&0\r\n", wind_speeds[current]);
    } else if (current > original)
        sprintf(buf, "&6The &7%s &6increases to a &7%s.&0\r\n", wind_speeds[original], wind_speeds[current]);
    else if (current == original)
        sprintf(buf, "&6A &7%s &6is blowing around you.&0\r\n", wind_speeds[current]);
    else if (current != WIND_NONE)
        sprintf(buf, "&6The &7%s &6subsides to a &7%s.&0\r\n", wind_speeds[original], wind_speeds[current]);
    else
        sprintf(buf, "&6The &7%s &6calms and the air becomes still.&0\r\n", wind_speeds[original]);
    return buf;
}

void update_wind(int zone_rnum) {
    int original, change;
    struct ZoneData *zone = &zone_table[zone_rnum];

    original = zone->wind_speed;

    /* Do we increment or decrement the wind? */
    switch (number(0, 4)) {
    case 0:
    case 1:
        break;
    case 2:
    case 3:
        zone->wind_speed--;
        break;
    default:
        zone->wind_speed++;
        break;
    }

    /* see if the wind dir changes, the following dirs are in no
     * specific order, just create a random sampling.
     */
    change = number(0, 9);
    if (change <= 3) /* 0 through 3 are cardinal directions */
        zone->wind_dir = change;

    /* Validate the wind value */
    if (zone->wind_speed > WIND_HURRICANE)
        zone->wind_speed = WIND_HURRICANE;
    else if (zone->wind_speed < WIND_NONE)
        zone->wind_speed = WIND_NONE;

    cbprintf(cb_outdoor, (void *)zone_rnum, "%s", wind_message(zone->wind_speed, original));
}

char *temperature_message(int temperature) {
    char *message;

    if (temperature <= 0)
        message = "It's too c-c-c-cold to be outside!\r\n";
    else if (temperature <= 30)
        message = "It's really c-c-c-cold!!\r\n";
    else if (temperature <= 50)
        message = "It's cold!\r\n";
    else if (temperature <= 65)
        message = "It's cool out here.\r\n";
    else if (temperature <= 75)
        message = "It's mild out today.\r\n";
    else if (temperature <= 90)
        message = "It's nice and warm out.\r\n";
    else if (temperature <= 100)
        message = "It's hot out here.\r\n";
    else if (temperature <= 150)
        message = "It's really, really hot here.\r\n";
    else
        message = "It's hotter than anyone could imagine!\r\n";
    return message;
}

void update_temperature(int zone_rnum) {
    int change;
    struct ZoneData *zone = &zone_table[zone_rnum];

    change = number(0, 5);

    if (number(0, 2))
        switch (zone->wind_speed) {
        case WIND_NONE:
            zone->temperature += change * 2;
            break;
        case WIND_BREEZE:
            zone->temperature += change;
            break;
        case WIND_STRONG:
        case WIND_GALE:
            zone->temperature -= change;
            break;
        case WIND_HURRICANE:
            zone->temperature -= change * 2;
            break;
        }

    /* Adjust for the sun... */
    switch (hemispheres[zone->hemisphere].sunlight) {
    case SUN_DARK:
        zone->temperature -= number(1, 5);
        break;
    case SUN_RISE:
        zone->temperature += number(1, 2);
        break;
    case SUN_LIGHT:
        zone->temperature += number(1, 5);
        break;
    case SUN_SET:
        zone->temperature -= number(1, 2);
        break;
    }

    /* Keep temperature in a sane range. */
    if (zone->temperature > BASE_TEMP(zone) + 30)
        zone->temperature = BASE_TEMP(zone) + 30 + number(1, 5);
    else if (zone->temperature < BASE_TEMP(zone) - 30)
        zone->temperature = BASE_TEMP(zone) - 30 - number(1, 5);

    /* Make sure temp falls in valid range */
    if (zone->temperature > TEMP_FIRE_PLANE)
        zone->temperature = TEMP_FIRE_PLANE;
    else if (zone->temperature < TEMP_FREEZING)
        zone->temperature = TEMP_FREEZING;

    cbprintf(cb_outdoor, (void *)zone_rnum, "%s", temperature_message(zone->temperature));
}

char *precipitation_message(ZoneData *zone, int original) {
    if (original > PRECIP_GRAY_CLOUDS) {
        if (zone->precipitation > original)
            sprintf(buf, "&9&8It starts %sing &9&8harder.&0\r\n", GET_PRECIP_TYPE(zone));
        else if (zone->precipitation == original)
            sprintf(buf, "&5It continues to %s.&0\r\n", GET_PRECIP_TYPE(zone));
        else if (zone->precipitation > PRECIP_GRAY_CLOUDS)
            sprintf(buf, "&5The %s &5starts coming down a little lighter.&0\r\n", GET_PRECIP_TYPE(zone));
        else
            sprintf(buf, "&5It continues to %s.&0\r\n", GET_PRECIP_TYPE(zone));
    } else if (original) {
        if (zone->precipitation <= PRECIP_GRAY_CLOUDS) {
            switch (original) {
            case PRECIP_PARTLY_CLOUDY:
                strcpy(buf,
                       "&4&8The sky is filled with small &7bil&0&7low&8ing white "
                       "&7c&0&7l&6ou&7d&8s.&0\r\n");
                break;
            case PRECIP_MOSTLY_CLOUDY:
                strcpy(buf,
                       "&7&bBil&0&7low&bing white &7c&0&7l&6ou&7d&bs &4cover the "
                       "sky.&0\r\n");
                break;
            case PRECIP_GRAY_CLOUDS:
                if (HEMISPHERE(zone).sunlight == SUN_DARK)
                    strcpy(buf,
                           "&9&bDark, ominous clouds&0 &4cover the sky, shrouding "
                           "the &7&8moon&8.&0\r\n");
                else
                    strcpy(buf,
                           "&9&bOminously dark clouds&0 &4fill the sky, blocking "
                           "out all &3sunlight.&0\r\n");
                break;
            default:
                /* Should not occur. */
                return "NULL PRECIPITATION\r\n";
            }
        } else if (zone->precipitation > PRECIP_GRAY_CLOUDS)
            sprintf(buf, "&9&8It begins to %s.&0\r\n", GET_PRECIP_TYPE(zone));
    } else if (zone->precipitation)
        strcpy(buf,
               "&4Small &7&bbil&0&7low&bing white &7c&0&7l&6ou&7d&bs&0 "
               "&4appear in the &bsky.&0\r\n");
    else
        switch (HEMISPHERE(zone).sunlight) {
        case SUN_LIGHT:
        case SUN_RISE:
            strcpy(buf, "&5The &3&bsun&0 &5shows &bbrightly&0 &5in the clear &4&bsky.&0\r\n");
            break;
        case SUN_DARK:
            strcpy(buf, "&5The &7moon&0 &5shines &8brightly&0 &5in the clear &4sky.&0\r\n");
            break;
        case SUN_SET:
            strcpy(buf, "&5The &3&8sun&0 &5glows &8&1red&0 &5in the clear &4sky.&0\r\n");
            break;
        default:
            /* Should not occur. */
            return "NULL PRECIPITATION\r\n";
        }
    return buf;
}

void update_precipitation(int zone_rnum) {
    int original, change;
    struct ZoneData *zone = &zone_table[zone_rnum];

    original = zone->precipitation;

    /* maintain our bias from wind_speed */
    change = number(0, 6) - zone->wind_speed;
    switch (change) {
    case 0:
    case 1:
        ++zone->precipitation;
        break;
    case 3:
    case 4:
        --zone->precipitation;
        break;
    default:
        /* no change in precipitation */
        break;
    }

    /* Make sure precipitation falls into valid range. */
    if (zone->precipitation > PRECIP_DANGEROUS)
        zone->precipitation = PRECIP_DANGEROUS;
    else if (zone->precipitation < PRECIP_NONE)
        zone->precipitation = PRECIP_NONE;

    cbprintf(cb_outdoor, (void *)zone_rnum, "%s", precipitation_message(zone, original));
}

/* Update the weather for all zones. */
void update_weather(long pulse) {
    int i;

    for (i = 0; i < top_of_zone_table; ++i) {
        if (zone_table[i].climate == CLIMATE_NONE)
            continue;
        switch (pulse % (3 * SECS_PER_MUD_HOUR * PASSES_PER_SEC)) {
        case (0 * SECS_PER_MUD_HOUR * PASSES_PER_SEC):
            update_wind(i);
            break;
        case (1 * SECS_PER_MUD_HOUR * PASSES_PER_SEC):
            update_temperature(i);
            break;
        case (2 * SECS_PER_MUD_HOUR * PASSES_PER_SEC):
            update_precipitation(i);
            break;
        default:
            log("Bad pulse value encountered in update_weather()");
            return;
        }
    }
}
