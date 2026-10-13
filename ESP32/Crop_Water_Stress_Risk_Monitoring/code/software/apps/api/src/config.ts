import { readFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { z } from "zod";
import dotenv from "dotenv";

// npm workspaces launch this service from apps/api; keep the sole editable .env at software/.env.
const envFile = process.env.DOTENV_CONFIG_PATH ?? resolve(process.cwd(), "../../.env");
dotenv.config({ path: envFile });

const optionalUrl = z.string().url().optional().or(z.literal(""));
const schema = z.object({
  NODE_ENV: z
    .enum(["development", "test", "staging", "production"])
    .default("development"),
  APP_URL: z.string().url(),
  API_URL: z.string().url(),
  CORS_ALLOWED_ORIGINS: z.string().min(1),
  COOKIE_DOMAIN: z.string().optional(),
  DATABASE_URL: z.string().url(),
  MQTT_BROKER_URL: z.string().url(),
  MQTT_USERNAME: z.string().min(1),
  MQTT_PASSWORD: z.string().min(1),
  MQTT_CA_PATH: z.string().optional(),
  MQTT_CA_PEM: z.string().optional(),
  SESSION_SECRET: z.string().min(32),
  ENCRYPTION_KEY: z.string().min(32),
  SMTP_HOST: z.string().optional(),
  SMTP_PORT: z.coerce.number().int().positive().optional(),
  SMTP_USERNAME: z.string().optional(),
  SMTP_PASSWORD: z.string().optional(),
  SMTP_FROM: z.string().optional(),
  OBJECT_STORAGE_ENDPOINT: optionalUrl,
  OBJECT_STORAGE_BUCKET: z.string().optional(),
  OBJECT_STORAGE_ACCESS_KEY: z.string().optional(),
  OBJECT_STORAGE_SECRET_KEY: z.string().optional(),
  WEATHER_LATITUDE: z.coerce.number().min(-90).max(90).optional(),
  WEATHER_LONGITUDE: z.coerce.number().min(-180).max(180).optional(),
  CONTRACT_DIR: z.string().optional(),
  PORT: z.coerce.number().int().positive().default(4000),
});
export type AppConfig = z.infer<typeof schema> & { mqttCa?: Buffer };
export function loadConfig(env = process.env): AppConfig {
  const config = schema.parse(env);
  if (
    config.NODE_ENV === "production" &&
    !config.APP_URL.startsWith("https://")
  )
    throw new Error("APP_URL must use HTTPS in production");
  if (
    config.MQTT_BROKER_URL.startsWith("mqtts://") &&
    !config.MQTT_CA_PATH &&
    !config.MQTT_CA_PEM
  )
    throw new Error("TLS MQTT requires MQTT_CA_PATH or MQTT_CA_PEM");
  return {
    ...config,
    CONTRACT_DIR: config.CONTRACT_DIR ? resolve(dirname(envFile), config.CONTRACT_DIR) : undefined,
    mqttCa: config.MQTT_CA_PEM
      ? Buffer.from(config.MQTT_CA_PEM.replace(/\\n/g, "\n"))
      : config.MQTT_CA_PATH
        ? readFileSync(config.MQTT_CA_PATH)
        : undefined,
  };
}
