import { useState }       from "react";
import {
  BarChart, Bar,
  XAxis, YAxis, CartesianGrid,
  Tooltip, ResponsiveContainer,
} from "recharts";
import { useSensorHistory }                    from "../hooks/useFirebase";
import { calcStats, exportCSV, METRIC_CONFIGS } from "../lib/utils";
import { Card, SectionTitle, NodeBadge, Icon, ChartTip } from "../components/shared/UI";
import type { NodeId, TimeRange }              from "../types";

export function StatsPage() {
  const [node,   setNode]   = useState<NodeId>("indoor");
  const [range,  setRange]  = useState<TimeRange>("24h");
  const [metric, setMetric] = useState<"pm25" | "pm10" | "current">("pm25");

  const { history } = useSensorHistory(node, 48);

  const SLICE: Record<TimeRange, number> = {
    "6h":  6,
    "12h": 12,
    "24h": history.length,
  };
  const filtered = history.slice(-SLICE[range]);
  const active   = METRIC_CONFIGS.find(m => m.key === metric)!;

  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 18 }}>

      {/* Node + Range selectors */}
      <div style={{ display: "flex", alignItems: "center", gap: 10, flexWrap: "wrap" }}>
        <NodeBadge nodeId="indoor"  active={node === "indoor"}  onClick={() => setNode("indoor")}  />
        <NodeBadge nodeId="outdoor" active={node === "outdoor"} onClick={() => setNode("outdoor")} />
        <div style={{ marginLeft: "auto", display: "flex", gap: 5 }}>
          {(["6h", "12h", "24h"] as TimeRange[]).map(r => (
            <button key={r} onClick={() => setRange(r)} style={{
              padding: "4px 10px", borderRadius: 6,
              border: "none", cursor: "pointer",
              background: range === r ? "#06b6d4" : "#1e293b",
              color:      range === r ? "#000"    : "#475569",
              fontSize: 10, fontFamily: "monospace",
            }}>{r}</button>
          ))}
        </div>
      </div>

      {/* Metric selector */}
      <div style={{ display: "flex", gap: 6 }}>
        {METRIC_CONFIGS.map(m => (
          <button key={m.key} onClick={() => setMetric(m.key)} style={{
            padding: "5px 14px", borderRadius: 6, cursor: "pointer",
            border: `1px solid ${metric === m.key ? m.color : "#334155"}`,
            background: metric === m.key ? m.color + "22" : "transparent",
            color:      metric === m.key ? m.color : "#475569",
            fontSize: 11, fontFamily: "monospace",
          }}>{m.label}</button>
        ))}
      </div>

      {/* Bar chart */}
      <Card>
        <div style={{ fontSize: 11, color: "#64748b", marginBottom: 12, fontFamily: "monospace" }}>
          {active.label} ({active.unit}) · {node} · {range}
        </div>
        <ResponsiveContainer width="100%" height={240}>
          <BarChart data={filtered}>
            <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
            <XAxis dataKey="time" stroke="#334155" tick={{ fontSize: 9, fill: "#475569" }} />
            <YAxis stroke="#334155" tick={{ fontSize: 9, fill: "#475569" }} />
            <Tooltip content={<ChartTip />} />
            <Bar dataKey={metric} fill={active.color} radius={[4,4,0,0]} name={active.label} />
          </BarChart>
        </ResponsiveContainer>
      </Card>

      {/* Summary table */}
      <Card>
        <SectionTitle icon="bar" label={`Summary · ${node} · ${range}`} />
        <table style={{ width: "100%", borderCollapse: "collapse", fontSize: 12 }}>
          <thead>
            <tr>
              {["Sensor", "Min", "Avg", "Max", "Unit", "n"].map(h => (
                <th key={h} style={{
                  padding: "7px 10px", textAlign: "left",
                  color: "#475569", letterSpacing: 1, fontSize: 10,
                }}>{h}</th>
              ))}
            </tr>
          </thead>
          <tbody>
            {METRIC_CONFIGS.map(({ key, label, unit, color }) => {
              const s = calcStats(filtered, key);
              return (
                <tr key={key} style={{ borderTop: "1px solid #1e293b" }}>
                  <td style={{ padding: "9px 10px", color, fontWeight: 700 }}>{label}</td>
                  <td style={{ padding: "9px 10px", color: "#64748b", fontFamily: "monospace" }}>{s.min}</td>
                  <td style={{ padding: "9px 10px", color: "#e2e8f0", fontFamily: "monospace", fontWeight: 700 }}>{s.avg}</td>
                  <td style={{ padding: "9px 10px", color: "#64748b", fontFamily: "monospace" }}>{s.max}</td>
                  <td style={{ padding: "9px 10px", color: "#334155" }}>{unit}</td>
                  <td style={{ padding: "9px 10px", color: "#475569" }}>{s.count}</td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </Card>

      {/* Export CSV */}
      <button onClick={() => exportCSV(filtered, `airnode_${node}_${range}_${Date.now()}.csv`)}
        style={{
          display: "flex", alignItems: "center", gap: 8,
          justifyContent: "center",
          background: "linear-gradient(135deg,#0891b2,#0284c7)",
          border: "none", color: "#fff", borderRadius: 10,
          padding: "12px 24px", cursor: "pointer",
          fontSize: 12, fontFamily: "monospace", fontWeight: 700,
          boxShadow: "0 0 18px #06b6d433",
        }}>
        <Icon name="download" size={15} color="#fff" />
        Export CSV — {filtered.length} rows
      </button>

    </div>
  );
}