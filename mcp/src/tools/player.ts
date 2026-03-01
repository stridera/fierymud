import { adminPost } from "../admin-client.js";

export const setPlayerFieldTool = {
  name: "set_player_field",
  description:
    "Set a player field directly (bypasses command system). Supported fields: wallet, bank, level, hp, maxhp, exp, alignment. For wallet/bank, value is in copper (e.g. '5000').",
  inputSchema: {
    type: "object" as const,
    properties: {
      player: {
        type: "string",
        description: "Name of the player to modify",
      },
      field: {
        type: "string",
        enum: ["wallet", "bank", "level", "hp", "maxhp", "exp", "alignment"],
        description: "The field to set",
      },
      value: {
        type: "string",
        description:
          "The value to set. For wallet/bank use copper amount (e.g. '5000'). For others use a number.",
      },
    },
    required: ["player", "field", "value"],
  },
  async handler(args: { player: string; field: string; value: string }) {
    return adminPost("/api/admin/player/set", {
      player_name: args.player,
      field: args.field,
      value: args.value,
    });
  },
};
