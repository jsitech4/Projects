import { existsSync, readdirSync, readFileSync } from "node:fs";
import { resolve } from "node:path";
import { createRequire } from "node:module";
const require = createRequire(import.meta.url);
const Ajv2020: any = require('ajv/dist/2020').default;
const addFormats: any = require('ajv-formats').default;

export type DeviceMessageKind =
  | "telemetry"
  | "events"
  | "command_responses"
  | "configuration";
const schemaFor: Record<DeviceMessageKind, string> = {
  telemetry: "telemetry",
  events: "event",
  command_responses: "command_response",
  configuration: "configuration",
};
export class ContractValidator {
  private ajv = addFormats(new Ajv2020({ allErrors: true, strict: false }));
  private validators = new Map<string, any>();
  constructor(
    contractDir = process.env.CONTRACT_DIR ??
      resolve(process.cwd(), "../hardware/schemas"),
  ) {
    if (!existsSync(contractDir))
      throw new Error(
        `Firmware schemas unavailable at ${contractDir}. Set CONTRACT_DIR to a copy of hardware/schemas.`,
      );
    for (const file of readdirSync(contractDir).filter((name) =>
      name.endsWith(".schema.json"),
    )) {
      const name = file.replace(".schema.json", "");
      this.validators.set(
        name,
        this.ajv.compile(
          JSON.parse(readFileSync(resolve(contractDir, file), "utf8")),
        ),
      );
    }
  }
  validate(kind: DeviceMessageKind, value: unknown): string[] {
    const validator = this.validators.get(schemaFor[kind]);
    if (!validator) return [`missing firmware schema: ${schemaFor[kind]}`];
    return validator(value)
      ? []
      : (validator.errors ?? []).map(
          (e: any) => `${e.instancePath || "/"} ${e.message}`,
        );
  }
}
export function parseDeviceTopic(
  topic: string,
): { deviceId: string; kind: DeviceMessageKind } | undefined {
  const match =
    /^cws\/v1\/devices\/([A-Za-z0-9_-]+)\/(telemetry|events|command_responses|configuration)$/.exec(
      topic,
    );
  return match
    ? { deviceId: match[1], kind: match[2] as DeviceMessageKind }
    : undefined;
}
export function deviceTime(value: any): Date | null {
  return value?.time?.synchronized && Number.isInteger(value?.time?.unix_s)
    ? new Date(value.time.unix_s * 1000)
    : null;
}
