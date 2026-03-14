import { Card, SectionTitle, Icon, Spinner } from "../shared/UI";
import type { ControlState, OperatingMode, DeviceCommand, NodeId } from "../../types";

// ── Fan speed labels ───────────────────────────────────────
const FAN_STEPS = ["OFF", "LOW", "MED", "HIGH", "MAX"] as const;

// ── Operating modes ────────────────────────────────────────
const MODES: OperatingMode[] = ["auto", "manual", "sleep"];

// ── One-shot commands ──────────────────────────────────────
const COMMANDS: Array<{ label: string; cmd: DeviceCommand; color: string }> = [
  { label: "SYNC NOW",    cmd: "sync",    color: "#22c55e" },
  { label: "RESTART MCU", cmd: "restart", color: "#f59e0b" },
  { label: "SLEEP MODE",  cmd: "sleep",   color: "#64748b" },
];

interface ControlPanelProps {
  nodeId:      NodeId;
  control:     ControlState;
  saving:      boolean;
  setFanSpeed: (v: number) => Promise<void>;
  setMode:     (v: OperatingMode) => Promise<void>;
  sendCommand: (cmd: DeviceCommand) => Promise<void>;
}

export function ControlPanel({
  nodeId, control, saving,
  setFanSpeed, setMode, sendCommand,
}: ControlPanelProps) {
  const nodeColor = nodeId === "indoor" ? "#06b6d4" : "#a78bfa";

  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 16 }}>

      {/* Saving indicator */}
      {saving && (
        <div style={{
          display: "flex", alignItems: "center", gap: 8,
          padding: "10px 16px", background: "#1e293b",
          borderRadius: 10, color: "#06b6d4",
          fontSize: 12, fontFamily: "monospace",
        }}>
          <Spinner size={14} />
          Writing to Firebase...
        </div>
      )}

      {/* Fan Speed */}
      <Card>
        <SectionTitle
          icon="fan" label="Fan Speed"
          color={nodeColor}
          sub={`/control/${nodeId}/fan_speed`}
        />

        <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 12 }}>
          <span style={{ color: "#64748b", fontSize: 12, fontFamily: "monospace" }}>
            Current
          </span>
          <span style={{ color: nodeColor, fontWeight: 700, fontSize: 18, fontFamily: "monospace" }}>
            {FAN_STEPS[control.fan_speed]}
          </span>
        </div>

        {/* Step buttons */}
        <div style={{ display: "flex", gap: 6, marginBottom: 14 }}>
          {FAN_STEPS.map((s, i) => (
            <button key={s} onClick={() => setFanSpeed(i)} style={{
              flex: 1, padding: "10px 0",
              borderRadius: 8, border: "none", cursor: "pointer",
              background: i <= control.fan_speed
                ? `rgba(6,182,212,${0.1 + i * 0.1})`
                : "#1e293b",
              color: i <= control.fan_speed ? nodeColor : "#475569",
              fontSize: 10, fontFamily: "monospace", fontWeight: 700,
              outline: i === control.fan_speed ? `1px solid ${nodeColor}` : "none",
              transition: "all .2s",
            }}>{s}</button>
          ))}
        </div>

        {/* Progress bar */}
        <div style={{ position: "relative", height: 5, background: "#1e293b", borderRadius: 99 }}>
          <div style={{
            position: "absolute", left: 0, top: 0,
            height: "100%", borderRadius: 99,
            width: `${(control.fan_speed / 4) * 100}%`,
            background: `linear-gradient(90deg, ${nodeColor}88, ${nodeColor})`,
            transition: "width .4s ease",
            boxShadow: `0 0 8px ${nodeColor}`,
          }} />
        </div>
      </Card>

      {/* Mode */}
      <Card>
        <SectionTitle
          label="Operating Mode"
          color="#94a3b8"
          sub={`/control/${nodeId}/mode`}
        />
        <div style={{ display: "flex", flexDirection: "column", gap: 6 }}>
          {MODES.map((m) => (
            <button key={m} onClick={() => setMode(m)} style={{
              display: "flex", alignItems: "center", gap: 10,
              padding: "10px 12px", borderRadius: 8,
              border: "none", cursor: "pointer",
              background: control.mode === m ? nodeColor + "22" : "#1e293b",
              color:      control.mode === m ? nodeColor : "#475569",
              borderLeft: control.mode === m
                ? `3px solid ${nodeColor}`
                : "3px solid transparent",
              fontSize: 11, fontFamily: "monospace",
              letterSpacing: 1, textTransform: "uppercase",
              transition: "all .2s", textAlign: "left",
            }}>
              <div style={{
                width: 6, height: 6, borderRadius: "50%",
                background: control.mode === m ? nodeColor : "#334155",
              }} />
              {m}
            </button>
          ))}
        </div>
      </Card>

      {/* Commands */}
      <Card accent="#475569">
        <SectionTitle
          icon="zap" label="Commands"
          color="#94a3b8"
          sub="ESP32 executes these immediately upon receiving"
        />
        <div style={{ display: "flex", gap: 8 }}>
          {COMMANDS.map(({ label, cmd, color }) => (
            <button key={cmd} onClick={() => sendCommand(cmd)} style={{
              flex: 1, padding: "11px 0",
              borderRadius: 8, border: `1px solid ${color}33`,
              background: color + "11", color,
              fontSize: 11, fontFamily: "monospace",
              cursor: "pointer", letterSpacing: 1,
              fontWeight: 700, transition: "all .2s",
            }}>{label}</button>
          ))}
        </div>
      </Card>

    </div>
  );
}