# Configuration persistence

`cws-config` stores two complete JSON snapshots. Each has a CRC32 prefix and configuration revision. A candidate is type/range validated, certificate checked, written into the inactive slot and read back. Only then does a separate NVS commit pointer select its slot/revision and record the previous revision.

Power loss before the pointer write leaves the old committed snapshot authoritative. Startup accepts only revisions named by that pointer; an uncommitted candidate is not promoted. If the active snapshot is corrupt, the matching previous snapshot is attempted. If neither validates, default settings load with a configuration fault; measurements and the administrator namespace remain intact.

Recovery from a corrupt snapshot first persists a pointer to the recovered revision before reusing the damaged slot. A failed pointer repair blocks candidate writes and retains the recovered values in RAM. Recovery to defaults uses a revision-zero pointer before the first replacement write. These steps prevent a stale pointer from selecting a new, uncommitted candidate after a second interruption.

`revision` and `calibration_revision` are read-only. Successful configuration commits increment revision; calibration-related commits also increment calibration revision. Invalid patches leave both configuration and revision unchanged. Configuration object keys are allowlisted by the default model; unknown fields and arbitrary GPIO settings are rejected.

Namespaces are independent: `cws-auth` holds the password verifier, `cws-commands` holds replay receipts, `cws-system` counts boots, and `cws-queue` reserves history cursor ranges. No credentials are written into LittleFS records. NVS integrity/atomicity does not imply encryption.

Calibration is centralized in the configuration object and shared validators. `config.get`/`export` sanitize secrets and replace them with presence flags. Public CA content is also omitted from these exports. `examples/configuration.json` shows the sanitized default shape.
