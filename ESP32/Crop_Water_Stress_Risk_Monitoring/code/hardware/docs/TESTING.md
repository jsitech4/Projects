# Verification procedure

Run from the firmware project root:

```powershell
pio run -e esp32_wroom_32u
pio test -e native
.\.venv\Scripts\python.exe tools/test_persistence.py
$env:CWS_TEST_BROWSER = 'chrome'
.\.venv\Scripts\python.exe -m pytest test/integration -q --junitxml=test-results/integration.xml
node --check data/portal/app.js
```

`test_core` covers locked pins, monotonic deadline rollover, AM2301A checksum/sign/range vectors, both soil orientations, invalid calibration, divider calculations/clipping, CRC32, typed patches, atomic rejection, credential redaction, password policy and command target/expiry checks.

`tools/test_persistence.py` compiles the actual production Configuration/Storage/Portal/Indicators sources with isolated host adapters. It injects commit-pointer failures, corrupt records, partial writes, rename failures, queue overflow and restart/purge cursor reuse. Recovery tests include a second interruption after selecting a fallback snapshot or defaults. HTTP tests cover response writes, backpressure, exact setup expiry, origin/host/interface rejection, duplicate content lengths, body bounds and uncommissioned fail-closed behavior. Indicator tests cover timer rollover, bounded activation, polarity/mode changes, disable, mute and fault priority. The certificate adapter intentionally rejects certificates; it does not validate TLS cryptography.

Python contract tests check JSON Schemas, examples, command bounds, forbidden GPIO commands, ingestion assertions, backend idempotence and private provisioning. Playwright drives the actual portal assets with isolated API fixtures: login errors, mandatory replacement, password retention, Wi-Fi/static/MQTT forms, scans, countdown closure and 320/768/1280 px layouts. It writes screenshots under ignored `test-results/portal`.

The browser channel is selectable with `CWS_TEST_BROWSER`. The default is `msedge`; the verified run uses installed Chrome because Edge exits during Playwright startup on this host. This is a test-runner selection, not a restriction on owner browsers.

Browser fixture results do not prove radio, NVS, TLS or sensor operation. The native adapters test application logic rather than the ESP32 filesystem driver. Physical acceptance must include five-minute AP shutdown with/without saved credentials, continued acquisition during authentication/trials, previous-password rejection after reboot, actual CA/hostname rejection, ACL denial, power-loss write recovery, divider clipping, LCD absence and peak heap during TLS.

The exact final host results and resource sizes are recorded in [Verification report](VERIFICATION_REPORT.md). Build artifacts and test outputs remain ignored. No USB upload or remote broker acceptance is implied by a successful host build.
