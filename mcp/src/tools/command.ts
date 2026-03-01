import { adminPost } from "../admin-client.js";

export const executeCommandTool = {
  name: "execute_command",
  description:
    "Execute any game command as a player and return the captured output. The player must have an active session (virtual or telnet).",
  inputSchema: {
    type: "object" as const,
    properties: {
      player: {
        type: "string",
        description: "Name of the player to execute the command as",
      },
      command: {
        type: "string",
        description: 'The full game command to execute (e.g., "look", "kill orc", "cast \'fireball\' orc")',
      },
    },
    required: ["player", "command"],
  },
  async handler(args: { player: string; command: string }) {
    return adminPost("/api/admin/command", {
      executor: args.player,
      command: args.command,
    });
  },
};
