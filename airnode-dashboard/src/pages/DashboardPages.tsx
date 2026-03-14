import { METRIC_CONFIGS, getAQIInfo, fmtDateTime } from "../lib/utils";
import { MetricCard }    from "../components/dashboard/MetricCard";
import { Card, SectionTitle, StatusDot, NodeBadge } from "../components/shared/UI";
import {
  useSensorData, useSensorHistory, useConnectionStatus,
} from "../hooks/useFirebase";
import type { NodeId } from "../types";
import { useState }    from "react";
import { PMChart } from "../components/dashboard/PMCharts";

export function DashboardPage() {
  const [node, setNode] = useState<NodeId>("indoor");

  const { data, loading, lastUpdated } = useSensorData(node);
  const { history, loading: histLoading } = useSensorHistory(node, 48);
  const connected = useConnectionStatus();
  const aqi = getAQIInfo(data?.pm25);

  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 18 }}>

      {/* ── Node selector ── */}
      <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
        <NodeBadge nodeId="indoor"  active={node === "indoor"}  onClick={() => setNode("indoor")}  />
        <NodeBadge nodeId="outdoor" active={node === "outdoor"} onClick={() => setNode("outdoor")} />

        <div style={{ marginLeft: "auto", display: "flex", alignItems: "center", gap: 8 }}>
          <StatusDot ok={connected} />
          <span style={{ fontSize: 10, color: "#64748b", fontFamily: "monospace" }}>
            {connected ? "LIVE" : "OFFLINE"}
          </span>
          {lastUpdated && (
            <span style={{ fontSize: 10, color: "#334155", fontFamily: "monospace" }}>
              · {fmtDateTime(lastUpdated.getTime())}
            </span>
          )}
        </div>
      </div>

      {/* ── AQI Badge ── */}
      {data && (
        <div style={{
          display: "inline-flex", alignItems: "center", gap: 10,
          background: aqi.bg, border: `1px solid ${aqi.color}44`,
          borderRadius: 10, padding: "10px 18px", alignSelf: "flex-start",
        }}>
          <div style={{
            width: 10, height: 10, borderRadius: "50%",
            background: aqi.color, boxShadow: `0 0 8px ${aqi.color}`,
          }} />
          <span style={{ color: aqi.color, fontSize: 12, fontWeight: 700, letterSpacing: 1 }}>
            {aqi.label}
          </span>
          <span style={{ color: aqi.color + "99", fontSize: 11 }}>
            PM2.5 = {data.pm25?.toFixed(1) ?? "N/A"} µg/m³
          </span>
        </div>
      )}

      {/* ── Metric Gauges ── */}
      <div style={{ display: "grid", gridTemplateColumns: "repeat(3,1fr)", gap: 12 }}>
        {METRIC_CONFIGS.map((config) => (
          <MetricCard key={config.key} config={config} data={data} loading={loading} />
        ))}
      </div>

      {/* ── PM Chart ── */}
      <PMChart history={history} loading={histLoading} nodeId={node} />

      {/* ── Latest reading ── */}
      <Card>
        <SectionTitle icon="activity" label="Latest Reading" sub="Firebase /sensors/latest" />
        {data
          ? (
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 10 }}>
              {[
                { label: "PM2.5",   value: data.pm25?.toFixed(1) ?? "N/A",    unit: "µg/m³", color: "#f43f5e" },
                { label: "PM10",    value: data.pm10?.toFixed(1) ?? "N/A",    unit: "µg/m³", color: "#fb923c" },
                { label: "Current", value: data.current?.toFixed(2) ?? "N/A", unit: "A",     color: "#4ade80" },
              ].map(({ label, value, unit, color }) => (
                <div key={label} style={{
                  background: "#0f172a", borderRadius: 10,
                  padding: "12px 16px", border: `1px solid ${color}22`,
                }}>
                  <div style={{ fontSize: 10, color: "#475569", letterSpacing: 2, marginBottom: 4 }}>
                    {label}
                  </div>
                  <div style={{ fontSize: 22, color, fontWeight: 700, fontFamily: "monospace" }}>
                    {value}
                    <span style={{ fontSize: 11, color: color + "88", marginLeft: 4 }}>{unit}</span>
                  </div>
                </div>
              ))}
            </div>
          )
          : (
            <div style={{ color: "#334155", fontSize: 12, fontFamily: "monospace" }}>
              {loading ? "Loading from Firebase..." : "Waiting for ESP32 data..."}
            </div>
          )
        }
      </Card>

    </div>
  );
}