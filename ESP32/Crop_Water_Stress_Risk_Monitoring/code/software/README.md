# Crop Water-Stress Risk Monitoring Platform

Self-hosted web, API, database, and MQTT ingestion services for the ESP32 crop water-stress monitor.

## Hardware telemetry boundary

The platform stores only measurements emitted by the firmware: air temperature and relative humidity (AM2301A), calibrated relative soil-moisture index, battery voltage, solar voltage, sensor quality/calibration state, connectivity, storage, and device health. It does not infer rainfall, light, pH, EC, nutrients, true VWC, charging current, crop yield, or harvest maturity. See [hardware data capabilities](docs/HARDWARE_DATA_CAPABILITIES.md).

## Requirements

- Node.js 22 LTS
- PostgreSQL 16
- MQTT broker for local development or MQTTS broker for production
- The firmware JSON schemas from `../hardware/schemas`

## Local setup

1. In `software`, copy `.env.example` to `.env` and replace development placeholders as needed.
2. Start local dependencies with `docker compose up -d postgres mosquitto`.
3. Run `npm install`, `npm run db:generate`, and `npm run db:migrate`.
4. Set `INITIAL_OWNER_EMAIL` and `INITIAL_OWNER_PASSWORD` in the current terminal, run `npm run db:seed`, then clear those variables.
5. Start the API with `npm run dev` and the web application with `npm run dev:web`.

The API listens on port 4000 and the web application normally uses port 3000.

## Verification

```powershell
npm run typecheck
npm test
npm run build
```

## Dashboard

The Crop Water Monitor device view is a data-driven single-device dashboard with the supplied desktop reference layout. It reads live telemetry and history through the authenticated API, while device configuration changes remain command-confirmed rather than optimistic. See [dashboard implementation](docs/DASHBOARD.md).

## Crop risk configuration

The relative soil-moisture channel is not VWC, and no universal crop threshold is used. Create a crop profile for an organization with locally validated `soilModerate`, `soilHigh`, `soilCritical`, `staleMinutes`, and `ruleVersion` values, then assign it to the plot. Until that profile and valid, calibrated live measurements exist, water-stress risk is shown as `UNKNOWN`.

The API exposes:

- `GET` / `POST` `/organizations/:organizationId/crop-profiles`
- `POST` `/plots/:id/crop-profile`

Only organization owners and managers can create profiles or assign them. Changes are audited.

## Deployment

See [deployment instructions](docs/DEPLOYMENT.md) and [architecture](docs/ARCHITECTURE.md). The bundled Mosquitto configuration is for local development only; use authenticated TLS, per-device ACLs, and a trusted CA in production.
