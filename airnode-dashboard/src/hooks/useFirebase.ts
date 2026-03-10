import { useState, useEffect }  from "react";
import {
  ref, onValue, off,
  update, set,
  query, limitToLast, orderByChild,
} from "firebase/database";

import { db } from "../lib/firebase";
import { fmtTime }  from "../lib/utils";
import type {
  SensorData, HistoryEntry,
  ControlState, NodeId,
  OperatingMode, DeviceCommand,
} from "../types";
import { getPaths } from "../lib/constants";

//  useSensorData
//  Subscribe /sensors/{node}/latest real-time
export function useSensorData(nodeId: NodeId) {
  const [data,        setData]    = useState<SensorData | null>(null);
  const [loading,     setLoading] = useState(true);
  const [error,       setError]   = useState<string | null>(null);
  const [lastUpdated, setUpdated] = useState<Date | null>(null);

  useEffect(() => {
    const dbRef = ref(db, getPaths(nodeId).LATEST);

    const unsub = onValue(
      dbRef,
      (snapshot) => {
        const val = snapshot.val() as SensorData | null;
        if (val) {
          setData(val);
          setUpdated(new Date());
          setError(null);
        }
        setLoading(false);
      },
      (err) => {
        setError(err.message);
        setLoading(false);
      }
    );

    return () => off(dbRef, "value", unsub);
  }, [nodeId]);   // re-subscribe ถ้า nodeId เปลี่ยน

  return { data, loading, error, lastUpdated };
}

//  useSensorHistory
//  Subscribe → /sensors/{node}/history (last N entries)
export function useSensorHistory(nodeId: NodeId, limit = 48) {
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const histRef = query(
      ref(db, getPaths(nodeId).HISTORY),
      orderByChild("timestamp"),
      limitToLast(limit)
    );

    const unsub = onValue(histRef, (snapshot) => {
      const entries: HistoryEntry[] = [];
      snapshot.forEach((child) => {
        const val = child.val() as SensorData;
        entries.push({
          ...val,
          time: fmtTime(val.timestamp),
        });
      });
      setHistory(entries);
      setLoading(false);
    });

    return () => off(histRef, "value", unsub);
  }, [nodeId, limit]);

  return { history, loading };
}

//  useControl
//  /control/{node} real-time read/write
const DEFAULT_CONTROL: ControlState = {
  fan_speed: 0,
  mode:      "auto",
};

export function useControl(nodeId: NodeId) {
  const [control, setControl] = useState<ControlState>(DEFAULT_CONTROL);
  const [loading, setLoading] = useState(true);
  const [saving,  setSaving]  = useState(false);

  useEffect(() => {
    const ctrlRef = ref(db, getPaths(nodeId).CONTROL);

    const unsub = onValue(ctrlRef, (snap) => {
      const val = snap.val() as Partial<ControlState> | null;
      if (val) setControl({ ...DEFAULT_CONTROL, ...val });
      setLoading(false);
    });

    return () => off(ctrlRef, "value", unsub);
  }, [nodeId]);

  const writeControl = async (updates: Partial<ControlState>) => {
    setSaving(true);
    try {
      await update(ref(db, getPaths(nodeId).CONTROL), {
        ...updates,
        updated_at: Date.now(),
      });
    } finally {
      setSaving(false);
    }
  };

  const setFanSpeed  = (v: number)         => writeControl({ fan_speed: v });
  const setMode      = (v: OperatingMode)  => writeControl({ mode: v });
  const sendCommand  = (cmd: DeviceCommand) =>
    writeControl({ command: cmd, updated_at: Date.now() });

  return { control, loading, saving, setFanSpeed, setMode, sendCommand };
}

//  useConnectionStatus
//  /.info/connected = Firebase built-in path | true if connected to Firebase server
export function useConnectionStatus(): boolean {
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    const connRef = ref(db, ".info/connected");
    const unsub   = onValue(connRef, (snap) => {
      setConnected(snap.val() === true);
    });
    return () => off(connRef, "value", unsub);
  }, []);

  return connected;
}