import { adminPost } from "../admin-client.js";

export const createSessionTool = {
  name: "create_session",
  description:
    "Create a virtual test player session. Loads the player from the database, sets god level, and places them in the world. No telnet connection needed.",
  inputSchema: {
    type: "object" as const,
    properties: {
      player_name: {
        type: "string",
        description: "Name of the player character to load from the database",
      },
    },
    required: ["player_name"],
  },
  async handler(args: { player_name: string }) {
    return adminPost("/api/admin/session/create", {
      player_name: args.player_name,
    });
  },
};

export const destroySessionTool = {
  name: "destroy_session",
  description: "Remove a virtual test player from the world and clean up their session.",
  inputSchema: {
    type: "object" as const,
    properties: {
      player_name: {
        type: "string",
        description: "Name of the virtual session player to remove",
      },
    },
    required: ["player_name"],
  },
  async handler(args: { player_name: string }) {
    return adminPost("/api/admin/session/destroy", {
      player_name: args.player_name,
    });
  },
};
