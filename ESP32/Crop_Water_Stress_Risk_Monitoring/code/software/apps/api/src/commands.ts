import { randomUUID } from "node:crypto";
import { Prisma, type PrismaClient } from "@prisma/client";
import type { MqttService } from "./mqtt.js";

const destructive = new Set([
  "device.reboot",
  "device.factory_reset",
  "storage.purge_acknowledged",
  "wifi.forget_network",
  "config.restore_defaults",
]);
const mutations = new Set([
  "device.reboot",
  "device.factory_reset",
  "device.clear_recoverable_faults",
  "device.sync_time",
  "config.patch",
  "config.restore_defaults",
  "environment.read_now",
  "soil.read_now",
  "battery.read_now",
  "solar.read_now",
  "telemetry.read_now",
  "telemetry.publish_now",
  "telemetry.set_intervals",
  "telemetry.retry_pending",
  "storage.retry_sync",
  "storage.set_retention",
]);
export class CommandService {
  constructor(
    private db: PrismaClient,
    private mqtt: MqttService,
  ) {}
  async send(input: {
    deviceId: string;
    command: string;
    args?: Record<string, unknown>;
    expectedRevision?: number;
    actorId?: string;
  }) {
    const args = input.args ?? {};
    if (
      destructive.has(input.command) &&
      args.confirm_device_id !== input.deviceId
    )
      throw new Error(
        "Destructive commands require confirm_device_id equal to the device ID.",
      );
    if (
      mutations.has(input.command) &&
      !Number.isInteger(input.expectedRevision)
    )
      throw new Error("Mutating commands require expectedRevision.");
    const expiresAt = new Date(Date.now() + 5 * 60_000);
    const requestId = randomUUID();
    const record = await this.db.command.create({
      data: {
        requestId,
        deviceId: input.deviceId,
        command: input.command,
        args: args as Prisma.InputJsonValue,
        expectedRevision: input.expectedRevision,
        expiresAt,
        createdById: input.actorId,
      },
    });
    try {
      await this.mqtt.publishCommand(input.deviceId, {
        schema_version: "1.0",
        request_id: requestId,
        device_id: input.deviceId,
        command: input.command,
        args,
        expires_at: Math.floor(expiresAt.getTime() / 1000),
        ...(mutations.has(input.command)
          ? { expected_revision: input.expectedRevision }
          : {}),
      });
    } catch (error) {
      await this.db.command.update({
        where: { id: record.id },
        data: { status: "FAILED", response: { error: String(error) } },
      });
      throw error;
    }
    return record;
  }
}
