# Hardware-faithful local device simulation

This package is separate from `hardware/` and `software/`. It registers one local demo device in the existing development PostgreSQL database, then behaves as a device over MQTT. Its messages use the real examples and contract structure in `../hardware`.

## Run

Start the local PostgreSQL, Mosquitto, web and API services first. Then, in a second terminal:

```bash
cd simulation
npm install
npm run normal
```

Other scenarios: `npm run dry-stress`, `npm run sensor-fault`, and `npm run offline-recovery`.

The device is named **North Field Sensor** in the database, but the MQTT payload makes no special “simulation” claim. Stop it with `Ctrl+C`. It writes the exact messages it published to `out/messages.jsonl` for troubleshooting only.

## One-command launcher

From the parent `code` folder, run `npm run simulation`. It starts the Docker services, applies migrations, starts the API/dashboard when needed, and runs the normal device scenario. Choose another scenario with `npm run simulation -- dry-stress` (or `sensor-fault`, `offline-recovery`). Docker Desktop must already be running. The first launch still requires the one-time `software/.env` and owner-account setup.

The simulator subscribes to the same device-specific `commands` and `sync_ack` topics. It returns contract-shaped command responses and logs durable acknowledgements, allowing the command centre and delivery workflow to be exercised without a physical ESP32.
