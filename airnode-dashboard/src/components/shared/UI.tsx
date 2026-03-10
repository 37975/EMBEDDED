import type { CSSProperties, ReactNode } from "react";

// ═══════════════════════════════════════════════════════════
//  Icon
// ═══════════════════════════════════════════════════════════
const PATHS: Record<string, string> = {
  home: "M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z",
  bar: "M18 20V10M12 20V4M6 20v-6",
  sliders:
    "M4 21v-7M4 10V3M12 21v-9M12 8V3M20 21v-5M20 12V3M1 14h6M9 8h6M17 16h6",
  wind: "M9.59 4.59A2 2 0 1 1 11 8H2m10.59 11.41A2 2 0 1 0 14 16H2m15.73-8.27A2.5 2.5 0 1 1 19.5 12H2",
  fan: "M12 12m-3 0a3 3 0 1 0 6 0 3 3 0 1 0-6 0M12 3a9 9 0 0 1 5 16.33M12 3a9 9 0 0 0-5 16.33",
  zap: "m13 2-3 14m0 0-3-6m3 6 3-6",
  activity: "M22 12h-4l-3 9L9 3l-3 9H2",
  download: "M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M7 10l5 5 5-5M12 15V3",
  refresh:
    "M23 4v6h-6M1 20v-6h6m16.83-3a10 10 0 0 1-18.8 4M1.17 11a10 10 0 0 1 18.8-4",
  check: "M20 6 9 17l-5-5",
  indoor: "M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z M9 22V12h6v10",
  outdoor:
    "M12 2a7 7 0 0 1 7 7c0 5-7 13-7 13S5 14 5 9a7 7 0 0 1 7-7z M12 11a2 2 0 1 0 0-4 2 2 0 0 0 0 4z",
};

interface IconProps {
  name: string;
  size?: number;
  color?: string;
}
export function Icon({ name, size = 16, color = "currentColor" }: IconProps) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      stroke={color}
      strokeWidth={2}
      strokeLinecap="round"
      strokeLinejoin="round"
      style={{ flexShrink: 0 }}
    >
      <path d={PATHS[name] ?? ""} />
    </svg>
  );
}

interface StatusDotProps {
  ok: boolean;
}
export function StatusDot({ ok }: StatusDotProps) {
  return (
    <span
      style={{
        display: "inline-block",
        width: 8,
        height: 8,
        borderRadius: "50%",
        flexShrink: 0,
        background: ok ? "#22c55e" : "#ef4444",
        boxShadow: ok ? "0 0 6px #22c55e" : "0 0 6px #ef4444",
        animation: ok ? "pulse 2s infinite" : "none",
      }}
    />
  );
}

// ═══════════════════════════════════════════════════════════
//  Toggle — on/off switch
// ═══════════════════════════════════════════════════════════
interface ToggleProps {
  value: boolean;
  onChange: (v: boolean) => void;
  onColor?: string;
}
export function Toggle({ value, onChange, onColor = "#22c55e" }: ToggleProps) {
  return (
    <div
      onClick={() => onChange(!value)}
      style={{
        width: 44,
        height: 24,
        borderRadius: 99,
        cursor: "pointer",
        background: value ? onColor : "#334155",
        position: "relative",
        transition: "background .3s",
        boxShadow: value ? `0 0 10px ${onColor}66` : "none",
        flexShrink: 0,
      }}
    >
      <div
        style={{
          position: "absolute",
          top: 3,
          left: value ? 23 : 3,
          width: 18,
          height: 18,
          borderRadius: "50%",
          background: "#fff",
          transition: "left .3s cubic-bezier(.4,0,.2,1)",
        }}
      />
    </div>
  );
}

// ═══════════════════════════════════════════════════════════
//  Card
// ═══════════════════════════════════════════════════════════
interface CardProps {
  children: ReactNode;
  accent?: string;
  style?: CSSProperties;
}
export function Card({ children, accent, style = {} }: CardProps) {
  return (
    <div
      style={{
        background: "linear-gradient(135deg,#0f172a,#1e293b)",
        border: `1px solid ${accent ? accent + "40" : "#1e293b"}`,
        borderRadius: 14,
        padding: 20,
        boxShadow: accent ? `0 0 24px ${accent}12` : "none",
        ...style,
      }}
    >
      {children}
    </div>
  );
}

// ═══════════════════════════════════════════════════════════
//  SectionTitle
// ═══════════════════════════════════════════════════════════
interface SectionTitleProps {
  icon?: string;
  label: string;
  sub?: string;
  color?: string;
}
export function SectionTitle({
  icon,
  label,
  sub,
  color = "#94a3b8",
}: SectionTitleProps) {
  return (
    <div
      style={{
        display: "flex",
        alignItems: "center",
        gap: 8,
        marginBottom: 18,
      }}
    >
      {icon && <Icon name={icon} size={14} color={color} />}
      <div>
        <div
          style={{
            fontSize: 11,
            fontWeight: 700,
            color,
            letterSpacing: 2,
            textTransform: "uppercase",
          }}
        >
          {label}
        </div>
        {sub && (
          <div style={{ fontSize: 10, color: "#475569", marginTop: 1 }}>
            {sub}
          </div>
        )}
      </div>
    </div>
  );
}

// ═══════════════════════════════════════════════════════════
//  GaugeArc — circular progress gauge
// ═══════════════════════════════════════════════════════════
interface GaugeArcProps {
  value: number | string;
  max: number;
  color: string;
  unit?: string;
  size?: number;
}
export function GaugeArc({
  value,
  max,
  color,
  unit = "",
  size = 108,
}: GaugeArcProps) {
  const pct = Math.min((parseFloat(String(value)) || 0) / max, 1);
  const R = 44;
  const cx = 60;
  const cy = 62;
  const start = -215;
  const end = 35;
  const angle = start + (end - start) * pct;
  const rad = (a: number) => (a * Math.PI) / 180;
  const arc = (a1: number, a2: number) => {
    const x1 = cx + R * Math.cos(rad(a1)),
      y1 = cy + R * Math.sin(rad(a1));
    const x2 = cx + R * Math.cos(rad(a2)),
      y2 = cy + R * Math.sin(rad(a2));
    return `M${x1} ${y1} A${R} ${R} 0 ${Math.abs(a2 - a1) > 180 ? 1 : 0} 1 ${x2} ${y2}`;
  };
  return (
    <svg width={size} height={size} viewBox="0 0 120 120">
      <path
        d={arc(start, end)}
        stroke="#1e293b"
        fill="none"
        strokeWidth={9}
        strokeLinecap="round"
      />
      <path
        d={arc(start, angle)}
        stroke={color}
        fill="none"
        strokeWidth={9}
        strokeLinecap="round"
        style={{
          filter: `drop-shadow(0 0 5px ${color}aa)`,
          transition: "all .5s ease",
        }}
      />
      <text
        x="60"
        y="59"
        textAnchor="middle"
        fill={color}
        fontSize="16"
        fontWeight="700"
        fontFamily="monospace"
      >
        {value ?? "—"}
      </text>
      <text
        x="60"
        y="73"
        textAnchor="middle"
        fill={color + "88"}
        fontSize="9"
        fontFamily="monospace"
      >
        {unit}
      </text>
    </svg>
  );
}

// ═══════════════════════════════════════════════════════════
//  Spinner
// ═══════════════════════════════════════════════════════════
interface SpinnerProps {
  size?: number;
  color?: string;
}
export function Spinner({ size = 18, color = "#06b6d4" }: SpinnerProps) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      stroke={color}
      strokeWidth={2}
      strokeLinecap="round"
      style={{ animation: "spin 1s linear infinite", flexShrink: 0 }}
    >
      <path d="M12 2v4M12 18v4M4.93 4.93l2.83 2.83M16.24 16.24l2.83 2.83M2 12h4M18 12h4M4.93 19.07l2.83-2.83M16.24 7.76l2.83-2.83" />
    </svg>
  );
}

// ═══════════════════════════════════════════════════════════
//  ChartTip — custom recharts tooltip
// ═══════════════════════════════════════════════════════════
interface ChartTipProps {
  active?: boolean;
  payload?: Array<{ color: string; name: string; value: number }>;
  label?: string;
}
export function ChartTip({ active, payload, label }: ChartTipProps) {
  if (!active || !payload?.length) return null;
  return (
    <div
      style={{
        background: "#0f172a",
        border: "1px solid #1e293b",
        borderRadius: 8,
        padding: "8px 12px",
        fontSize: 11,
        fontFamily: "monospace",
      }}
    >
      <div style={{ color: "#64748b", marginBottom: 4 }}>{label}</div>
      {payload.map((p, i) => (
        <div key={i} style={{ color: p.color }}>
          {p.name}: <strong>{p.value}</strong>
        </div>
      ))}
    </div>
  );
}

// ═══════════════════════════════════════════════════════════
//  NodeBadge — แสดง indoor/outdoor
// ═══════════════════════════════════════════════════════════
interface NodeBadgeProps {
  nodeId: "indoor" | "outdoor";
  active: boolean;
  onClick: () => void;
}
export function NodeBadge({ nodeId, active, onClick }: NodeBadgeProps) {
  const color = nodeId === "indoor" ? "#06b6d4" : "#a78bfa";
  return (
    <button
      onClick={onClick}
      style={{
        display: "flex",
        alignItems: "center",
        gap: 6,
        padding: "7px 14px",
        borderRadius: 8,
        border: "none",
        background: active ? color + "22" : "#1e293b",
        color: active ? color : "#475569",
        outline: active ? `1px solid ${color}` : "none",
        cursor: "pointer",
        fontSize: 11,
        fontFamily: "monospace",
        fontWeight: 700,
        letterSpacing: 1,
        transition: "all .2s",
        textTransform: "uppercase",
      }}
    >
      <Icon name={nodeId} size={13} color={active ? color : "#475569"} />
      {nodeId}
    </button>
  );
}
