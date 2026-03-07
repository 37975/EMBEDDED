export type OperatingMode = 'auto' | 'manual' | 'eco' | 'boost';
export type DeviceCommand = 'sync' | 'restart' | 'flush' | 'ota' | 'sleep' | 'reset' | '';
export type TimeRange = "6h" | "12h" | "24h" | "7d" | "30d";
export type ChartMetricKey = keyof Pick<
  SensorData,
  "pm25" | "pm10" | "temperature" | "humidity" | "pressure" | "current"
>;

export interface SensorData {
  pm25:        number;
  pm10:        number;
  temperature: number;
  humidity:    number;
  pressure:    number;
  current:     number; // กระแสไฟฟ้า
  timestamp:   number;
}

export interface HistoryEntry extends SensorData {
  time: string;
}

export interface ControlState {
    fan_speed : number;
    mode: OperatingMode;
    alerts_enabled: boolean;
    command?: DeviceCommand;
    command_at?: number;
    update_at?: number;
}