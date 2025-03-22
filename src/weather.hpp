/***************************************************************************
 *   File: weather.h                                      Part of FieryMUD *
 *  Usage: Weather structures, defines and prototypes.  Most of            *
 *         the weather system design is documented herein as well.         *
 *     By: Tim Holcomb (Fingh on Hubis)                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"
#include "zone.hpp"

/* Zone climate types */
#define CLIMATE_NONE 0        /* do not report weather */
#define CLIMATE_SEMIARID 1    /* plains */
#define CLIMATE_ARID 2        /* deserts */
#define CLIMATE_OCEANIC 3     /* ocean */
#define CLIMATE_TEMPERATE 4   /* mediterranean climate */
#define CLIMATE_SUBTROPICAL 5 /* florida */
#define CLIMATE_TROPICAL 6    /* equatorial / jungle */
#define CLIMATE_SUBARCTIC 7   /* high elevation */
#define CLIMATE_ARCTIC 8      /* extreme polar */
#define CLIMATE_ALPINE 9      /* mountain */
#define NUM_CLIMATES 10
/* add more climate types here, and add initialization
 * for them in climates[] */

/* Seasons */
#define WINTER 0
#define SPRING 1
#define SUMMER 2
#define AUTUMN 3

/* Temperature Presets */
#define TEMP_FRIGID (-10)
#define TEMP_FREEZING 10
#define TEMP_COLD 30
#define TEMP_COOL 50
#define TEMP_MILD 65
#define TEMP_WARM 75
#define TEMP_HOT 90
#define TEMP_STEAMING 100
#define TEMP_BLAZING 110
#define TEMP_FIRE_PLANE 200

/* precipitation rates */
#define PRECIP_NONE 0          // not a cloud in the sky, sunny
#define PRECIP_PARTLY_CLOUDY 1 // a few clouds, a little sun
#define PRECIP_MOSTLY_CLOUDY 2 // no sunny day...
#define PRECIP_GRAY_CLOUDS 3   // wow, it's gonna rain/snow!
#define PRECIP_DRIZZLE 4       // snow flurries or drizzly rain
#define PRECIP_LIGHT 5         // trace amounts of rain or snow
#define PRECIP_ACTIVE 6        // hey, it's raining or snowing!
#define PRECIP_HEAVY 7         // it's really coming down now!
#define PRECIP_DANGEROUS 8 // most likely accompanied by WIND_GALE or higher, and probably will result in disaster soon

/* keep track of which messages to send to users. */
#define PRECIP_TYPE_RAIN 0
#define PRECIP_TYPE_SNOW 1

/* Wind speeds */
#define WIND_NONE 0
#define WIND_BREEZE 1
#define WIND_STRONG 2
#define WIND_GALE 3
#define WIND_HURRICANE 4 // if accompanied by precipitation, things are gonna get ugly...

/* Disaster types */
#define DISASTER_NONE 0
#define DISASTER_TORNADO (1 << 0)
#define DISASTER_BLIZZARD (1 << 1)
#define DISASTER_EARTHQUAKE (1 << 2)
#define DISASTER_FLOOD (1 << 3)
#define DISASTER_HAILSTORM (1 << 4)
#define DISASTER_SANDSTORM (1 << 5)
#define DISASTER_HEATWAVE (1 << 6)
#define DISASTER_HURRICANE (1 << 7)
#define DISASTER_TSUNAMI (1 << 8)
#define DISASTER_WATERSPOUT (1 << 9)

/* offset parameters based on season and climate type */
#define SUMMER_TEMP 25
#define SUMMER_PREC (-2)
#define WINTER_TEMP (-30)
#define WINTER_PREC 1
#define SPRING_TEMP 12
#define SPRING_PREC 2
#define AUTUMN_TEMP (-15)
#define AUTUMN_PREC (-1)

/* utility macros */
#define PRECIP_TYPE(temp) (temp > TEMP_COLD ? PRECIP_TYPE_RAIN : PRECIP_TYPE_SNOW)
#define GET_PRECIP_TYPE(zone) precip[PRECIP_TYPE((zone)->temperature)]
#define CLIMATE(zone) ((zone)->climate)
#define BASE_TEMP(zone) climates[CLIMATE(zone)].base_temperature
#define BASE_PRECIP(zone) climates[CLIMATE(zone)].base_precipitation_rate;
#define BASE_WIND(zone) climates[CLIMATE(zone)].base_wind_speed
#define HEMISPHERE(zone) hemispheres[(zone)->hemisphere]
#define DISASTER(zone) zone->disaster_type
#define IN_HEMISPHERE(ch) zone_table[IN_ZONE_RNUM(ch)].hemisphere

/* start nature in motion */
void init_weather(void);

void update_weather(long pulse);
void increment_game_time();
void update_daylight();
void update_season();

char *wind_message(int current_wind, int original_wind);
char *temperature_message(int temperature);
char *precipitation_message(ZoneData *zone, int original);

/* hemispheres for weather and time NE< NW< SE< SW */
struct HemisphereData {
    const char *name;
    int season;
    int sunlight;
};

/* climate structs are initialized in an array, where the index
 * corresponds to the climate type as defined below in CLIMATE_xxx
 */
struct ClimateData {
    const char *name;
    int base_temperature;
    int base_precipitation_rate;
    int base_wind_speed;
    int allowed_disasters;
};

extern const char *wind_speeds[];
extern const char *precip[];
extern const char *daylight_change[];
extern const char *seasons[];
extern const char *season_change[];
extern HemisphereData hemispheres[NUM_HEMISPHERES];
extern ClimateData climates[NUM_CLIMATES];