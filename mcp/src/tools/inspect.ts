import { adminGet } from "../admin-client.js";

export const lookRoomTool = {
  name: "look_room",
  description:
    "Get structured room information: name, description, exits, actors present, objects on ground.",
  inputSchema: {
    type: "object" as const,
    properties: {
      zone_id: { type: "number", description: "Zone ID of the room" },
      id: { type: "number", description: "Local ID of the room within the zone" },
    },
    required: ["zone_id", "id"],
  },
  async handler(args: { zone_id: number; id: number }) {
    return adminGet(`/api/admin/room/${args.zone_id}/${args.id}`);
  },
};

export const inspectActorTool = {
  name: "inspect_actor",
  description:
    "Get live actor state: current HP/mana, position, room, effects, combat state, stats. Works for both players and mobs.",
  inputSchema: {
    type: "object" as const,
    properties: {
      name: {
        type: "string",
        description: "Name of the actor (player or mob) to inspect",
      },
    },
    required: ["name"],
  },
  async handler(args: { name: string }) {
    return adminGet(`/api/admin/actor/${encodeURIComponent(args.name)}`);
  },
};

export const inspectMobTool = {
  name: "inspect_mob",
  description:
    "Get mob prototype stats: HP, combat stats, aggro condition, race, description. This shows the template, not a live instance.",
  inputSchema: {
    type: "object" as const,
    properties: {
      zone_id: { type: "number", description: "Zone ID of the mob prototype" },
      id: { type: "number", description: "Local ID of the mob within the zone" },
    },
    required: ["zone_id", "id"],
  },
  async handler(args: { zone_id: number; id: number }) {
    return adminGet(`/api/admin/mob/${args.zone_id}/${args.id}`);
  },
};
