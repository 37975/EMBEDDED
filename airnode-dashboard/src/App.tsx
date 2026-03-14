import { useState } from "react";
import { ControlPage }   from "./pages/ControlPage";
import { StatsPage }     from "./pages/StatsPage";
import { Icon, StatusDot } from "./components/shared/UI";
import { useConnectionStatus } from "./hooks/useFirebase";
import { DashboardPage } from "./pages/DashboardPages";

type TabKey = "dashboard" | "control" | "stats";

const TABS: Array<{ key: TabKey; label: string; icon: string }> = [
  { key: "dashboard", label: "Dashboard", icon: "home"    },
  { key: "control",   label: "Control",   icon: "sliders" },
  { key: "stats",     label: "Statistics",icon: "bar"     },
];

export default function App() {
  const [tab, setTab] = useState<TabKey>("dashboard");
  const connected     = useConnectionStatus();

  return (
    <div style={{
      minHeight: "100vh",
      background: "#020617",
      color: "#e2e8f0",
      fontFamily: "'Courier New', Courier, monospace",
      display: "flex",
      flexDirection: "column",
    }}>

      {/* ── Header ───────────────────────────────────────── */}
      <header style={{
        background: "linear-gradient(180deg,#0f172a,#020617)",
        borderBottom: "1px solid #1e293b",
        padding: "12px 24px",
        display: "flex", alignItems: "center",
        justifyContent: "space-between",
        position: "sticky", top: 0, zIndex: 100,
      }}>
        {/* Logo */}
        <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
          <div style={{
            width: 36, height: 36, borderRadius: 9,
            background: "linear-gradient(135deg,#0891b2,#7c3aed)",
            display: "flex", alignItems: "center", justifyContent: "center",
            boxShadow: "0 0 14px #06b6d422",
          }}>
            <Icon name="wind" size={17} color="#fff" />
          </div>
          <div>
            <div style={{ fontSize: 14, fontWeight: 700, letterSpacing: 2 }}>
              AIRNODE
            </div>
            <div style={{ fontSize: 9, color: "#475569", letterSpacing: 2 }}>
              AIR QUALITY MONITOR
            </div>
          </div>
        </div>

        {/* Firebase status */}
        <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
          <StatusDot ok={connected} />
          <span style={{ fontSize: 10, color: "#64748b" }}>
            {connected ? "FIREBASE LIVE" : "OFFLINE"}
          </span>
        </div>
      </header>

      {/* ── Nav ──────────────────────────────────────────── */}
      <nav style={{
        background: "#0f172a",
        borderBottom: "1px solid #1e293b",
        display: "flex",
        padding: "0 24px",
        gap: 2,
      }}>
        {TABS.map(t => (
          <button key={t.key} onClick={() => setTab(t.key)} style={{
            display: "flex", alignItems: "center", gap: 6,
            padding: "11px 16px",
            background: "transparent", border: "none",
            borderBottom: tab === t.key
              ? "2px solid #06b6d4"
              : "2px solid transparent",
            color: tab === t.key ? "#06b6d4" : "#475569",
            cursor: "pointer",
            fontSize: 11, fontFamily: "monospace",
            letterSpacing: 1, transition: "all .2s",
          }}>
            <Icon
              name={t.icon} size={13}
              color={tab === t.key ? "#06b6d4" : "#475569"}
            />
            {t.label.toUpperCase()}
          </button>
        ))}
      </nav>

      {/* ── Page ─────────────────────────────────────────── */}
      <main style={{
        flex: 1,
        padding: "20px 24px",
        maxWidth: 1200,
        margin: "0 auto",
        width: "100%",
        boxSizing: "border-box",
      }}>
        {tab === "dashboard" && <DashboardPage />}
        {tab === "control"   && <ControlPage />}
        {tab === "stats"     && <StatsPage />}
      </main>

      {/* ── Global styles ────────────────────────────────── */}
      <style>{`
        * { box-sizing: border-box; }
        body { margin: 0; }
        @keyframes pulse {
          0%,100% { opacity: 1; }
          50%      { opacity: .4; }
        }
        @keyframes spin { to { transform: rotate(360deg); } }
        input[type=range] {
          -webkit-appearance: none;
          height: 4px; border-radius: 99px;
          background: #1e293b; outline: none; cursor: pointer;
        }
        input[type=range]::-webkit-slider-thumb {
          -webkit-appearance: none;
          width: 14px; height: 14px;
          border-radius: 50%; cursor: pointer;
        }
        button { transition: opacity .15s, transform .15s; }
        button:hover:not(:disabled) { opacity: .85; transform: translateY(-1px); }
        button:disabled { opacity: .4; cursor: not-allowed !important; }
        ::-webkit-scrollbar { width: 5px; }
        ::-webkit-scrollbar-track { background: #0f172a; }
        ::-webkit-scrollbar-thumb { background: #1e293b; border-radius: 3px; }
      `}</style>

    </div>
  );
}