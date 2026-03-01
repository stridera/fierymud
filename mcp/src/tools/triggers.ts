import { adminGet, adminPost } from "../admin-client.js";

export const triggerErrorsTool = {
  name: "trigger_errors",
  description:
    "List all triggers that have needsReview=true (failing triggers). Shows trigger ID, name, error type, and message.",
  inputSchema: {
    type: "object" as const,
    properties: {
      recent: {
        type: "boolean",
        description: "If true, show recent error log entries instead of triggers needing review",
      },
    },
  },
  async handler(args: { recent?: boolean }) {
    if (args.recent) {
      return adminGet("/api/admin/triggers/errors/recent");
    }
    return adminGet("/api/admin/triggers/errors");
  },
};

export const triggerInfoTool = {
  name: "trigger_info",
  description:
    "Get full trigger details: Lua source code, flags, variables, recent error history.",
  inputSchema: {
    type: "object" as const,
    properties: {
      zone_id: { type: "number", description: "Zone ID of the trigger" },
      id: { type: "number", description: "Local ID of the trigger within the zone" },
    },
    required: ["zone_id", "id"],
  },
  async handler(args: { zone_id: number; id: number }) {
    return adminGet(`/api/admin/triggers/${args.zone_id}/${args.id}`);
  },
};

export const fireTriggerTool = {
  name: "fire_trigger",
  description:
    "Manually execute a trigger for debugging. Returns the trigger result (Continue/Halt/Error) and captured output.",
  inputSchema: {
    type: "object" as const,
    properties: {
      zone_id: { type: "number", description: "Zone ID of the trigger" },
      id: { type: "number", description: "Local ID of the trigger" },
      actor: {
        type: "string",
        description: "Name of the actor triggering the event (e.g., test player name)",
      },
      target: {
        type: "string",
        description: "Name of the mob/actor that owns the trigger",
      },
      speech: {
        type: "string",
        description: "Speech text for SPEECH triggers",
      },
    },
    required: ["zone_id", "id"],
  },
  async handler(args: {
    zone_id: number;
    id: number;
    actor?: string;
    target?: string;
    speech?: string;
  }) {
    const body: Record<string, unknown> = {};
    if (args.actor) body.actor = args.actor;
    if (args.target) body.target = args.target;
    if (args.speech) body.speech = args.speech;
    return adminPost(`/api/admin/triggers/${args.zone_id}/${args.id}/fire`, body);
  },
};

export const reloadTriggersTool = {
  name: "reload_triggers",
  description:
    "Reload all triggers for a zone from the database. Use after editing trigger source code.",
  inputSchema: {
    type: "object" as const,
    properties: {
      zone_id: {
        type: "number",
        description: "Zone ID to reload triggers for",
      },
    },
    required: ["zone_id"],
  },
  async handler(args: { zone_id: number }) {
    return adminPost(`/api/admin/triggers/reload/${args.zone_id}`, {});
  },
};

export const triggerStatsTool = {
  name: "trigger_stats",
  description: "Get trigger execution statistics: total, successful, halted, failed, yielded counts.",
  inputSchema: {
    type: "object" as const,
    properties: {},
  },
  async handler() {
    return adminGet("/api/admin/triggers/stats");
  },
};
