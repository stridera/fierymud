import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
} from "@modelcontextprotocol/sdk/types.js";

import { createSessionTool, destroySessionTool } from "./tools/session.js";
import { executeCommandTool } from "./tools/command.js";
import { lookRoomTool, inspectActorTool, inspectMobTool } from "./tools/inspect.js";
import {
  triggerErrorsTool,
  triggerInfoTool,
  fireTriggerTool,
  reloadTriggersTool,
  triggerStatsTool,
} from "./tools/triggers.js";
import { setPlayerFieldTool } from "./tools/player.js";
import {
  pauseWorldTool,
  unpauseWorldTool,
  tickTool,
  worldStatusTool,
  spawnEntityTool,
  teleportTool,
} from "./tools/world-control.js";

interface ToolDef {
  name: string;
  description: string;
  inputSchema: Record<string, unknown>;
  handler: (args: Record<string, unknown>) => Promise<unknown>;
}

// Collect all tools
const allTools: ToolDef[] = [
  // Session management
  createSessionTool,
  destroySessionTool,
  // Command execution
  executeCommandTool,
  // Inspection
  lookRoomTool,
  inspectActorTool,
  inspectMobTool,
  // Triggers
  triggerErrorsTool,
  triggerInfoTool,
  fireTriggerTool,
  reloadTriggersTool,
  triggerStatsTool,
  // Player management
  setPlayerFieldTool,
  // World control
  pauseWorldTool,
  unpauseWorldTool,
  tickTool,
  worldStatusTool,
  spawnEntityTool,
  teleportTool,
] as unknown as ToolDef[];

const toolMap = new Map<string, ToolDef>();
for (const tool of allTools) {
  toolMap.set(tool.name, tool);
}

const server = new Server(
  { name: "fierymud", version: "1.0.0" },
  { capabilities: { tools: {} } },
);

server.setRequestHandler(ListToolsRequestSchema, async () => ({
  tools: allTools.map((t) => ({
    name: t.name,
    description: t.description,
    inputSchema: t.inputSchema,
  })),
}));

server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const tool = toolMap.get(request.params.name);
  if (!tool) {
    return {
      content: [{ type: "text" as const, text: `Unknown tool: ${request.params.name}` }],
      isError: true,
    };
  }

  try {
    const result = await tool.handler((request.params.arguments ?? {}) as Record<string, unknown>);
    return {
      content: [
        {
          type: "text" as const,
          text: JSON.stringify(result, null, 2),
        },
      ],
    };
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    return {
      content: [
        {
          type: "text" as const,
          text: JSON.stringify({ error: message }, null, 2),
        },
      ],
      isError: true,
    };
  }
});

// Start the server with stdio transport
const transport = new StdioServerTransport();
await server.connect(transport);
