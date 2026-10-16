import mqtt, { type MqttClient } from 'mqtt';
import type { AppConfig } from './config.js';
import { IngestionService } from './ingestion.js';

export class MqttService {
  private client?: MqttClient;
  constructor(private config: AppConfig, private ingestion: IngestionService, private log: { info: Function; warn: Function; error: Function }) {}
  start() {
    this.client = mqtt.connect(this.config.MQTT_BROKER_URL, { username: this.config.MQTT_USERNAME, password: this.config.MQTT_PASSWORD, ca: this.config.mqttCa, rejectUnauthorized: true, clean: true, reconnectPeriod: 5_000 });
    this.client.on('connect', () => { this.log.info('MQTT connected'); this.client!.subscribe(['cws/v1/devices/+/telemetry', 'cws/v1/devices/+/events', 'cws/v1/devices/+/command_responses', 'cws/v1/devices/+/configuration'], { qos: 1 }); });
    this.client.on('message', async (topic, message) => {
      try {
        const saved = await this.ingestion.ingest(topic, message);
        if (saved) this.publishDurableAck(saved.deviceId, saved.messageId); // only executes after the transaction returned successfully
      } catch (error) { this.log.warn({ topic, error }, 'MQTT message rejected'); }
    });
    this.client.on('error', (error) => this.log.error({ error }, 'MQTT connection error'));
  }
  async publishCommand(deviceId: string, payload: object): Promise<void> { await this.publish(`cws/v1/devices/${deviceId}/commands`, payload); }
  private publishDurableAck(deviceId: string, messageId: string) { void this.publish(`cws/v1/devices/${deviceId}/sync_ack`, { schema_version: '1.0', device_id: deviceId, message_id: messageId, durably_stored: true }).catch((error) => this.log.error({ error, messageId }, 'ack publish failed')); }
  private publish(topic: string, payload: object): Promise<void> { return new Promise((resolve, reject) => this.client?.publish(topic, JSON.stringify(payload), { qos: 1, retain: false }, (e) => e ? reject(e) : resolve()) ?? reject(new Error('MQTT client unavailable'))); }
  stop() { this.client?.end(true); }
}
