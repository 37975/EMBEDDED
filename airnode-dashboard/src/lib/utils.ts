import type { AQIInfo, HistoryEntry } from "../types";

// AQI PM2.5
export function getAQIInfo(pm25: number | null | undefined): AQIInfo {
  if (pm25 == null) return { label: "N/A",               color: "#64748b", bg: "#1e293b" };
  if (pm25 <= 50)   return { label: "Good",              color: "#22c55e", bg: "#052e16" };
  if (pm25 <= 100)  return { label: "Moderate",          color: "#eab308", bg: "#1c1700" };
  if (pm25 <= 150)  return { label: "Unhealthy (Sens.)", color: "#f97316", bg: "#1c0e00" };
  if (pm25 <= 200)  return { label: "Unhealthy",         color: "#ef4444", bg: "#200000" };
  if (pm25 <= 300)  return { label: "Very Unhealthy",    color: "#a855f7", bg: "#180023" };
  return                   { label: "Hazardous",         color: "#7f1d1d", bg: "#1a0000" };
}

export interface Stats {
  min: string; max: string; avg: string; count: number;
}
export function calcStats(arr: HistoryEntry[], key: keyof HistoryEntry): Stats {
  const vals = arr
    .map(d => d[key] as number)
    .filter(v => typeof v === "number" && !isNaN(v));
  if (!vals.length) return { min: "—", max: "—", avg: "—", count: 0 };
  return {
    min:   Math.min(...vals).toFixed(1),
    max:   Math.max(...vals).toFixed(1),
    avg:   (vals.reduce((a, b) => a + b, 0) / vals.length).toFixed(1),
    count: vals.length,
  };
}

// Export CSV
export function exportCSV(data: HistoryEntry[], filename = "airnode.csv"): void {
  const headers: (keyof HistoryEntry)[] = [
    "time", "timestamp", "pm25", "pm10", "current", "node_id"
  ];
  const rows = data.map(r => headers.map(h => r[h] ?? "").join(","));
  const csv  = [headers.join(","), ...rows].join("\n");
  const url  = URL.createObjectURL(new Blob([csv], { type: "text/csv" }));
  Object.assign(document.createElement("a"), { href: url, download: filename }).click();
  URL.revokeObjectURL(url);
}

// ── Format timestamp → "HH:MM" ────────────────────────────────
export function fmtTime(ts: number | Date | null): string {
  if (!ts) return "--:--";
  return new Date(ts).toLocaleTimeString("th-TH", {
    hour: "2-digit", minute: "2-digit",
  });
}

// ── Format timestamp → "วัน เดือน HH:MM" ─────────────────────
export function fmtDateTime(ts: number | null): string {
  if (!ts) return "—";
  return new Date(ts).toLocaleString("th-TH", {
    day: "numeric", month: "short",
    hour: "2-digit", minute: "2-digit", second: "2-digit",
  });
}

// ── Metric configs ────────────────────────────────────────────
import type { MetricConfig } from "../types";

export const METRIC_CONFIGS: MetricConfig[] = [
  { key: "pm25",    label: "PM2.5",   unit: "µg/m³", color: "#f43f5e", max: 200 },
  { key: "pm10",    label: "PM10",    unit: "µg/m³", color: "#fb923c", max: 300 },
  { key: "current", label: "Current", unit: "A",     color: "#4ade80", max: 5   },
];