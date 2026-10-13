# Crop Water-Stress Monitoring Platform

Self-hosted dashboard, API and durable MQTT receiver for the adjacent ESP32 firmware. The hardware contract in `../hardware` is authoritative.

## Structure

- `apps/api`: Fastify API, Prisma database model, MQTT ingestion/command service, risk engine.
- `apps/web`: Next.js operator dashboard.
- `packages/contracts`: contract ownership and deployment guidance.
- `deploy`: local broker and reverse-proxy examples.
- `docs`: architecture and production operations.

Read [Deployment](docs/DEPLOYMENT.md) before exposing anything to the internet. The starting configuration uses local development services only.
