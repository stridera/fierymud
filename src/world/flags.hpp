#pragma once

/** Zone flags affecting behavior */
enum class ZoneFlag {
    Closed = 0,  // Zone is closed to players
    NoMortals,   // Only immortals can enter
    Quest,       // Quest zone
    Grid,        // Grid-based movement
    Maze,        // Maze zone
    Recall_Ok,   // Allow recall in zone
    Summon_Ok,   // Allow summoning in zone
    Teleport_Ok, // Allow teleporting in zone
    Search,      // Zone allows searching
    Noattack,    // No combat allowed
    Worldmap,    // Zone is part of world map
    Astral,      // Astral plane zone

    // Clan zones
    ClanZone, // Zone belongs to a clan

    // Special zones
    Newbie, // Newbie zone
    Arena,  // Arena zone
    Prison, // Prison zone

    // Weather/environment
    NoWeather,   // Zone not affected by weather
    Underground, // Underground zone

    // PK zones
    ChaosOk, // PK allowed
    LawfulOk // Only lawful PK allowed
};

/** Room sector types affecting movement and behavior */
enum class SectorType {
    Inside = 0,
    City,
    Field,
    Forest,
    Hills,
    Mountains,
    Water_Swim,
    Water_Noswim,
    Underwater,
    Flying,
    Desert,
    Swamp,
    Beach,
    Road,
    Underground,
    Lava,
    Ice,
    Astral,
    Fire,
    Lightning,
    Spirit,
    Badlands,
    Void,

    // Sentinel
    Undefined
};
