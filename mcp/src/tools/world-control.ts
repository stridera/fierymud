import { adminGet, adminPost } from "../admin-client.js";

export const pauseWorldTool = {
  name: "pause_world",
  description:
    "Freeze all game systems: combat rounds, regen ticks, mob AI, triggers. World state becomes static until unpaused or ticked.",
  inputSchema: {
    type: "object" as const,
    properties: {},
  },
  async handler() {
    return adminPost("/api/admin/world/pause", {});
  },
};

export const unpauseWorldTool = {
  name: "unpause_world",
  description:
    "Resume normal real-time operation after pausing. All game systems restart their timers.",
  inputSchema: {
    type: "object" as const,
    properties: {},
  },
  async handler() {
    return adminPost("/api/admin/world/unpause", {});
  },
};

export const tickTool = {
  name: "tick",
  description:
    "Advance the game by N ticks while paused. Processes combat rounds, casting, regen, mob AI. Returns all output generated during the ticks.",
  inputSchema: {
    type: "object" as const,
    properties: {
      count: {
        type: "number",
        description: "Number of ticks to advance (default: 1)",
      },
    },
  },
  async handler(args: { count?: number }) {
    return adminPost("/api/admin/world/tick", {
      count: args.count ?? 1,
    });
  },
};

export const worldStatusTool = {
  name: "world_status",
  description: "Check if the world is currently paused or running.",
  inputSchema: {
    type: "object" as const,
    properties: {},
  },
  async handler() {
    return adminGet("/api/admin/world/status");
  },
};

export const spawnEntityTool = {
  name: "spawn_entity",
  description:
    "Spawn a mob or object instance into the world at a specific room. Useful for setting up test scenarios.",
  inputSchema: {
    type: "object" as const,
    properties: {
      type: {
        type: "string",
        enum: ["mob", "object"],
        description: "Whether to spawn a mob or an object",
      },
      zone_id: {
        type: "number",
        description: "Zone ID of the mob/object prototype",
      },
      id: {
        type: "number",
        description: "Local ID of the mob/object prototype within the zone",
      },
      room_zone: {
        type: "number",
        description: "Zone ID of the room to spawn in",
      },
      room_id: {
        type: "number",
        description: "Local ID of the room to spawn in",
      },
    },
    required: ["type", "zone_id", "id", "room_zone", "room_id"],
  },
  async handler(args: {
    type: string;
    zone_id: number;
    id: number;
    room_zone: number;
    room_id: number;
  }) {
    return adminPost("/api/admin/spawn", {
      type: args.type,
      zone_id: args.zone_id,
      id: args.id,
      room_zone: args.room_zone,
      room_id: args.room_id,
    });
  },
};

export const teleportTool = {
  name: "teleport",
  description:
    "Move a player to a specific room. The player must have an active session (virtual or telnet).",
  inputSchema: {
    type: "object" as const,
    properties: {
      player: {
        type: "string",
        description: "Name of the player to teleport",
      },
      zone_id: {
        type: "number",
        description: "Zone ID of the destination room",
      },
      room_id: {
        type: "number",
        description: "Local ID of the destination room",
      },
    },
    required: ["player", "zone_id", "room_id"],
  },
  async handler(args: { player: string; zone_id: number; room_id: number }) {
    return adminPost("/api/admin/teleport", {
      player_name: args.player,
      zone_id: args.zone_id,
      room_id: args.room_id,
    });
  },
};
