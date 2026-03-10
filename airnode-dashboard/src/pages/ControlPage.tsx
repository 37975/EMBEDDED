import { useState }        from "react";
import { useControl }      from "../hooks/useFirebase";
import { ControlPanel }    from "../components/control/ControlPanel";
import { NodeBadge }       from "../components/shared/UI";
import type { NodeId }     from "../types";

export function ControlPage() {
  const [node, setNode] = useState<NodeId>("indoor");

  const { control, saving, setFanSpeed, setMode, sendCommand } = useControl(node);

  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 18 }}>

      {/* Node selector */}
      <div style={{ display: "flex", gap: 10 }}>
        <NodeBadge nodeId="indoor"  active={node === "indoor"}  onClick={() => setNode("indoor")}  />
        <NodeBadge nodeId="outdoor" active={node === "outdoor"} onClick={() => setNode("outdoor")} />
      </div>

      <ControlPanel
        nodeId={node}
        control={control}
        saving={saving}
        setFanSpeed={setFanSpeed}
        setMode={setMode}
        sendCommand={sendCommand}
      />

    </div>
  );
}