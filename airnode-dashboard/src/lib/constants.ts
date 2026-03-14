import type { NodeId } from "../types";

// ── Firebase paths แยกตาม node ──────────────────────────────
export const DB_PATHS = {
  indoor: {
    LATEST:      "sensors/indoor/latest",
    HISTORY:     "sensors/indoor/history",
    CONTROL:     "control/indoor",
  },
  outdoor: {
    LATEST:      "sensors/outdoor/latest",
    HISTORY:     "sensors/outdoor/history",
    CONTROL:     "control/outdoor",
  },
  CONNECTION: ".info/connected",
} as const;

export function getPaths(nodeId: NodeId) {
  return DB_PATHS[nodeId];
}