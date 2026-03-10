import { Card, GaugeArc, Spinner } from "../shared/UI";
import type { MetricConfig, SensorData } from "../../types";

interface MetricCardProps {
  config:  MetricConfig;
  data:    SensorData | null;
  loading: boolean;
}

export function MetricCard({ config, data, loading }: MetricCardProps) {
  const { key, label, unit, color, max } = config;
  const value = data ? Number(data[key]).toFixed(1) : null;

  return (
    <Card accent={color} style={{
      display: "flex", flexDirection: "column",
      alignItems: "center", gap: 4, padding: 14,
    }}>
      <span style={{
        fontSize: 10, color, letterSpacing: 2,
        fontFamily: "monospace", textTransform: "uppercase",
      }}>
        {label}
      </span>

      {loading
        ? <div style={{ height: 108, display: "flex", alignItems: "center" }}>
            <Spinner color={color} />
          </div>
        : <GaugeArc value={value ?? "—"} max={max} color={color} unit={unit} />
      }
    </Card>
  );
}