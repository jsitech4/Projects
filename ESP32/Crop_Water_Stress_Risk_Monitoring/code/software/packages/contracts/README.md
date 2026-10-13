# Firmware contracts

The authoritative versioned JSON schemas are maintained in `../../hardware/schemas`.
Set `CONTRACT_DIR` to that directory in development, and copy the same unmodified directory into the deployment artifact or mounted secrets/config volume for production. The API deliberately fails to start without it, preventing silent protocol drift.
