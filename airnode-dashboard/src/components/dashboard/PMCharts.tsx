import { useState } from "react";
import {
  AreaChart, Area,
  XAxis, YAxis, CartesianGrid,
  Tooltip, Legend, ResponsiveContainer,
} from "recharts";
import { Card, SectionTitle, ChartTip } from "../shared/UI";
import type { HistoryEntry, TimeRange }  from "../../types";

interface PMChartProps {
  history: HistoryEntry[];
  loading: boolean;
  nodeId:  string;
}

export function PMChart({ history, loading, nodeId }: PMChartProps) {
  const [range, setRange] = useState<TimeRange>("24h");

  const SLICE: Record<TimeRange, number> = {
    "6h":  6,
    "12h": 12,
    "24h": history.length,
  };
  const data = history.slice(-SLICE[range]);

  return (
    <Card>
      {/* Header */}
      <div style={{
        display: "flex", justifyContent: "space-between",
        alignItems: "center", marginBottom: 16,
      }}>
        <SectionTitle
          icon="wind"
          label="PM2.5 & PM10"
          sub={`real-time · ${nodeId}`}
        />
        {/* Range buttons */}
        <div style={{ display: "flex", gap: 5 }}>
          {(["6h", "12h", "24h"] as TimeRange[]).map((r) => (
            <button key={r} onClick={() => setRange(r)} style={{
              padding: "4px 10px", borderRadius: 6,
              border: "none", cursor: "pointer",
              background: range === r ? "#06b6d4" : "#1e293b",
              color:      range === r ? "#000"    : "#475569",
              fontSize: 10, fontFamily: "monospace",
              transition: "all .2s",
            }}>{r}</button>
          ))}
        </div>
      </div>

      {/* Chart */}
      {loading || !data.length
        ? <div style={{
            height: 200, display: "flex",
            alignItems: "center", justifyContent: "center",
            color: "#334155", fontSize: 12,
          }}>
            {loading ? "Loading..." : "Waiting for ESP32 data..."}
          </div>
        : <ResponsiveContainer width="100%" height={200}>
            <AreaChart data={data}>
              <defs>
                <linearGradient id="gPM25" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%"  stopColor="#f43f5e" stopOpacity={0.3} />
                  <stop offset="95%" stopColor="#f43f5e" stopOpacity={0}   />
                </linearGradient>
                <linearGradient id="gPM10" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%"  stopColor="#fb923c" stopOpacity={0.2} />
                  <stop offset="95%" stopColor="#fb923c" stopOpacity={0}   />
                </linearGradient>
              </defs>
              <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
              <XAxis dataKey="time"
                stroke="#334155"
                tick={{ fontSize: 9, fill: "#475569" }} />
              <YAxis
                stroke="#334155"
                tick={{ fontSize: 9, fill: "#475569" }} />
              <Tooltip content={<ChartTip />} />
              <Legend wrapperStyle={{ fontSize: 10, color: "#64748b" }} />
              <Area type="monotone" dataKey="pm25"
                stroke="#f43f5e" strokeWidth={2}
                fill="url(#gPM25)" name="PM2.5" dot={false} />
              <Area type="monotone" dataKey="pm10"
                stroke="#fb923c" strokeWidth={2}
                fill="url(#gPM10)" name="PM10"  dot={false} />
            </AreaChart>
          </ResponsiveContainer>
      }
    </Card>
  );
}