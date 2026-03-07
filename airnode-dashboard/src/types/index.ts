export interface SensorData {
  pm25:      number;
  pm10:      number;
  current:   number;
  node_id:   string;
  timestamp: number;
}

export interface HistoryEntry extends SensorData {
  time: string;   // "14:30" — format จาก timestamp ใน hook
}

export interface ControlState {
  fan_speed:  number;          // 0–4
  mode:       OperatingMode;
  command?:   DeviceCommand;
  command_at?: number;
  updated_at?: number;
}


// export interface CalibrationOffsets {
//   pm25_offset: number;
//   pm10_offset: number;
//   updated_at?: number;
// }

export type OperatingMode = "auto" | "manual" | "sleep";
export type DeviceCommand = "sync" | "restart" | "sleep" | "";
export type NodeId        = "indoor" | "outdoor";
export type TimeRange     = "6h" | "12h" | "24h";

// AQI info
export interface AQIInfo {
  label: string;
  color: string;
  bg:    string;
}

// Config metric
export interface MetricConfig {
  key:   keyof Pick<SensorData, "pm25" | "pm10" | "current">;
  label: string;
  unit:  string;
  color: string;
  max:   number;
}