# Deployment and credential changes

## First local run

1. From `software`, copy `.env.example` to `.env` and set non-placeholder values.
2. Set `CONTRACT_DIR=../hardware/schemas` for local development. Production must ship the exact same unmodified folder and set its absolute container path.
3. Run `npm install`, `docker compose up -d postgres mosquitto`, `npm run db:generate`, then `npm run db:migrate`.
4. Create the first owner once: set `INITIAL_OWNER_EMAIL` and `INITIAL_OWNER_PASSWORD` in the terminal only, run `npm run db:seed`, then remove those environment variables.
5. Run `npm run dev -w @cws/api` and `npm run dev -w @cws/web`.

The included Mosquitto service is development-only: it is anonymous and unencrypted. Do not expose it publicly.

## When domain and hosting are purchased

No code needs to change. Set deployment secrets/environment variables: `APP_URL`, `API_URL`, `CORS_ALLOWED_ORIGINS`, `COOKIE_DOMAIN`, `DATABASE_URL`, `MQTT_BROKER_URL`, `MQTT_USERNAME`, `MQTT_PASSWORD`, one MQTT CA option, `SESSION_SECRET`, and `ENCRYPTION_KEY`. Apply migrations before deploying the new API. Put the web/API behind Caddy or Nginx and use HTTPS.

Use separate staging and production databases, backend MQTT service accounts, and secret stores. Rotate a secret by replacing it in the host, rolling the affected service, verifying health, then revoking the old credential. Never put Wi-Fi/router passwords in this platform.

## MQTT production checklist

- Use an `mqtts://` hostname and a public/private CA trusted by the backend and devices.
- Create a distinct device credential per registered device and deliver it only through the ESP32 local setup portal.
- Device ACL: publish only its six device topics; subscribe only its own `commands` and `sync_ack` topics.
- Backend ACL: subscribe to device-originated topics and publish commands/acknowledgements. Deny cross-device access to device identities.
- In a controlled setup, send `storage.set_retention` with `durable_ack: true`. This makes the firmware purge records only after the backend has committed them.

## Backup and restore

Nightly run `pg_dump --format=custom "$DATABASE_URL" > cws-YYYY-MM-DD.dump`, encrypt the file, and copy it to access-controlled storage. Test restores regularly using `pg_restore --clean --if-exists --dbname="$RESTORE_DATABASE_URL" cws-YYYY-MM-DD.dump`. Keep database backups separate from MQTT broker configuration and TLS keys.
