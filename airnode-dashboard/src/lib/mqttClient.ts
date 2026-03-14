import mqtt from "mqtt";
import type { MqttClient } from "mqtt";

const BROKER    = "wss://broker.hivemq.com:8884/mqtt";
const CLIENT_ID = "airnode_dashboard_" + Math.random().toString(36).slice(2, 7);

let client: MqttClient | null = null;

export function mqttConnect(
  onMessage?: (topic: string, payload: string) => void
): MqttClient {
  // ── ถ้า connect อยู่แล้ว return ตัวเดิม ──────────────────
  if (client) {
    if (onMessage) client.on("message", (t, p) => onMessage(t, p.toString()));
    return client;
  }

  client = mqtt.connect(BROKER, {
    clientId:        CLIENT_ID,
    clean:           true,
    reconnectPeriod: 3000,
  });

  client.on("connect", () => {
    console.log("[MQTT] connected:", CLIENT_ID);
  });

  client.on("message", (topic, payload) => {
    onMessage?.(topic, payload.toString());
  });

  client.on("error", (err) => {
    console.error("[MQTT] error:", err.message);
  });

  client.on("close", () => {
    console.warn("[MQTT] disconnected");
  });

  return client;
}

export function mqttPublish(topic: string, payload: string): void {
  if (!client?.connected) {
    console.warn("[MQTT] not connected");
    return;
  }
  client.publish(topic, payload);
  console.log(`[MQTT] → ${topic}: ${payload}`);
}

export function getMqttClient(): MqttClient | null {
  return client;
}