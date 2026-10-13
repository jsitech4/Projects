import type { PrismaClient } from '@prisma/client';
import { calculateRisk } from './risk.js';
import { ContractValidator, deviceTime, parseDeviceTopic, type DeviceMessageKind } from './contracts.js';

export class IngestionService {
  constructor(private db: PrismaClient, private contracts: ContractValidator) {}
  async ingest(topic: string, raw: Buffer): Promise<{ deviceId: string; messageId: string; duplicate: boolean } | undefined> {
    const route = parseDeviceTopic(topic); if (!route) return undefined;
    let payload: any; try { payload = JSON.parse(raw.toString('utf8')); } catch { throw new Error('invalid_json'); }
    const expectedMessageType: Record<DeviceMessageKind, string> = { telemetry: 'telemetry', status: 'status', events: 'event', command_responses: 'command_response', configuration: 'configuration', capabilities: 'capabilities' };
    if (payload.device_id !== route.deviceId || payload.message_type !== expectedMessageType[route.kind]) throw new Error('topic_envelope_mismatch');
    const errors = this.contracts.validate(route.kind, payload); if (errors.length) throw new Error(`schema_invalid: ${errors.join('; ')}`);
    const existing = await this.db.rawMessage.findUnique({ where: { messageId: payload.message_id } });
    if (existing) return { deviceId: route.deviceId, messageId: payload.message_id, duplicate: true };
    const receivedAt = new Date();
    await this.db.$transaction(async (tx) => {
      const device = await tx.device.findUnique({ where: { id: route.deviceId } });
      if (!device) throw new Error('unregistered_device');
      await tx.rawMessage.create({ data: { messageId: payload.message_id, deviceId: route.deviceId, topic, messageType: payload.message_type, payload, deviceTime: deviceTime(payload) } });
      await tx.device.update({ where: { id: route.deviceId }, data: { lastSeenAt: receivedAt, firmwareVersion: payload.firmware_version ?? undefined, hardwareModel: payload.hardware_model ?? undefined, lastConfigurationRevision: payload.configuration_revision ?? undefined } });
      if (route.kind === 'telemetry') {
        const m = payload.measurements;
        const record = await tx.telemetry.create({ data: { messageId: payload.message_id, deviceId: route.deviceId, sampledAt: deviceTime(payload), temperatureC: m.environment.temperature_c, humidityPct: m.environment.humidity_pct, relativeMoisturePct: m.soil.relative_moisture_pct, batteryV: m.battery.voltage_v, solarV: m.solar.voltage_v, valid: { soil: m.soil.valid, environment: m.environment.valid, battery: m.battery.valid, solar: m.solar.valid }, raw: payload } });
        const risk = calculateRisk({ temperatureC: record.temperatureC, humidityPct: record.humidityPct, relativeMoisturePct: record.relativeMoisturePct, batteryV: record.batteryV, soilValid: m.soil.valid, environmentValid: m.environment.valid, sampledAt: record.sampledAt, ingestedAt: record.ingestedAt, batteryCritical: m.battery.critical, faults: payload.health.active_faults });
        await tx.riskAssessment.create({ data: { deviceId: route.deviceId, telemetryId: record.id, level: risk.level, ruleVersion: risk.ruleVersion, factors: risk.factors, explanation: risk.explanation } });
      } else if (route.kind === 'events') await tx.deviceEvent.create({ data: { messageId: payload.message_id, deviceId: route.deviceId, code: payload.code, payload, occurredAt: deviceTime(payload) } });
      else if (route.kind === 'command_responses') await tx.command.updateMany({ where: { requestId: payload.request_id, deviceId: route.deviceId }, data: { status: payload.status.toUpperCase(), response: payload } });
      else await tx.deviceSnapshot.upsert({ where: { deviceId_kind: { deviceId: route.deviceId, kind: route.kind } }, create: { deviceId: route.deviceId, kind: route.kind, payload }, update: { payload, receivedAt } });
    });
    return { deviceId: route.deviceId, messageId: payload.message_id, duplicate: false };
  }
}
