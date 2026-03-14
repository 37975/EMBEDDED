import { useState, useEffect } from "react";
import { mqttConnect, mqttPublish } from "../lib/mqttClient";
import { Card, SectionTitle, NodeBadge } from "../components/shared/UI";
import type { NodeId, OperatingMode } from "../types";

// ── Mode config ────────────────────────────────────────────
const MODES: Array<{
  key:   OperatingMode;
  label: string;
  desc:  string;
  color: string;
}> = [
  { key: "auto",   label: "AUTO",   color: "#06b6d4", desc: "" },
  { key: "manual", label: "MANUAL", color: "#f59e0b", desc: "" },
  { key: "sleep",  label: "SLEEP",  color: "#64748b", desc: "" },
];

export function ControlPage() {
  const [node,      setNode]      = useState<NodeId>("indoor");
  const [mode,      setModeState] = useState<OperatingMode>("auto");
  const [connected, setConnected] = useState(false);
  const [lastSent,  setLastSent]  = useState<string | null>(null);

  // ── เชื่อม MQTT ────────────────────────────────────────────
  useEffect(() => {
  const c = mqttConnect((topic, payload) => {
    console.log(`[MQTT] ← ${topic}: ${payload}`);
  });

  // mqtt.js มี event "connect" และ "close" ให้ใช้โดยตรง
  const onConnect    = () => setConnected(true);
  const onDisconnect = () => setConnected(false);

  c.on("connect", onConnect);
  c.on("close",   onDisconnect);

  return () => {
    c.off("connect",    onConnect);
    c.off("close",      onDisconnect);
  };
}, []);

  // ── เปลี่ยน mode → publish MQTT ───────────────────────────
  const handleModeChange = (newMode: OperatingMode) => {
    setModeState(newMode);
    const topic = `airnode/${node}/mode`;
    mqttPublish(topic, newMode);
    setLastSent(`${topic} → "${newMode}"`);
  };

  const nodeColor = node === "indoor" ? "#06b6d4" : "#a78bfa";

  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 16 }}>

      {/* Node selector */}
      <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
        <NodeBadge nodeId="indoor"  active={node === "indoor"}  onClick={() => setNode("indoor")}  />
        <NodeBadge nodeId="outdoor" active={node === "outdoor"} onClick={() => setNode("outdoor")} />

        {/* MQTT status */}
        <div style={{ marginLeft: "auto", display: "flex", alignItems: "center", gap: 6 }}>
          <div style={{
            width: 7, height: 7, borderRadius: "50%",
            background:  connected ? "#22c55e" : "#ef4444",
            boxShadow:   connected ? "0 0 6px #22c55e" : "none",
          }} />
          <span style={{ fontSize: 10, color: "#64748b", fontFamily: "monospace" }}>
            MQTT {connected ? "CONNECTED" : "CONNECTING..."}
          </span>
        </div>
      </div>

      {/* Mode selector */}
      <Card>
        <SectionTitle
          label="Operating Mode"
          color={nodeColor}
          sub={`publish → airnode/${node}/mode`}
        />

        <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
          {MODES.map(({ key, label, desc, color }) => (
            <button key={key} onClick={() => handleModeChange(key)} style={{
              display: "flex", alignItems: "center", gap: 14,
              padding: "14px 16px", borderRadius: 10,
              border: "none", cursor: "pointer", textAlign: "left",
              background: mode === key ? color + "18" : "#0f172a",
              borderLeft: `3px solid ${mode === key ? color : "#1e293b"}`,
              transition: "all .2s",
            }}>
              {/* indicator */}
              <div style={{
                width: 10, height: 10, borderRadius: "50%", flexShrink: 0,
                background: mode === key ? color : "#1e293b",
                boxShadow:  mode === key ? `0 0 8px ${color}` : "none",
                transition: "all .2s",
              }} />

              <div style={{ flex: 1 }}>
                <div style={{
                  color: mode === key ? color : "#475569",
                  fontSize: 12, fontWeight: 700,
                  fontFamily: "monospace", letterSpacing: 2,
                }}>{label}</div>
                <div style={{ color: "#334155", fontSize: 10, marginTop: 2 }}>
                  {desc}
                </div>
              </div>

              {mode === key && (
                <span style={{
                  fontSize: 9, color, fontFamily: "monospace",
                  background: color + "22", padding: "3px 8px",
                  borderRadius: 4, letterSpacing: 1,
                }}>ACTIVE</span>
              )}
            </button>
          ))}
        </div>
      </Card>

      {/* Last sent */}
      {lastSent && (
        <div style={{
          background: "#0f172a", borderRadius: 8,
          padding: "10px 14px", border: "1px solid #1e293b",
          fontSize: 11, fontFamily: "monospace", color: "#22c55e",
        }}>
          ✓ {lastSent}
        </div>
      )}

    </div>
  );
}