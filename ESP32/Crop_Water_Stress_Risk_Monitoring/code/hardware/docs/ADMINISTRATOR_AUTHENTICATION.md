# Administrator authentication

The local username is `admin`. Setup-network authentication and administrator authentication use separate credentials. First commissioning authenticates against the private factory verifier and permits only password replacement or logout.

Replacement passwords contain 12–96 printable ASCII characters with uppercase, lowercase, a digit and punctuation. Confirmation must match, and the replacement must differ from the current password. The supplied password is verified before replacement. PBKDF2-HMAC-SHA256 uses 60,000 iterations and a fresh random 16-byte salt. The NVS verifier blob becomes authoritative only after a successful write; a stored verifier never falls back to the factory password.

Hash computation runs in a separate idle-priority task. HTTP starts a job and polls it with a random 256-bit job ID. Completed unclaimed jobs expire after 30 seconds; setup shutdown cancels all jobs. The application task commits results, avoiding races with NVS or session state.

Failed authentication incurs exponential throttling up to 60 seconds. One authentication job runs at a time. Sessions and CSRF tokens contain 256 random bits. Cookies use `HttpOnly`, `SameSite=Strict`, `Path=/` and at most 300 seconds of lifetime. A session never outlives the server's setup window. HTTP over the local WPA2 AP does not use a `Secure` cookie attribute because the service itself is HTTP.

Password replacement invalidates prior sessions and preserves connectivity, calibration and records. Power cycling does not reset authentication. Physical-service recovery requires the separate per-device service token:

The privileged calibration password is evaluated by the ordinary sign-in field. Its immutable verifier is compiled into the firmware image and is not kept as a plaintext source value, NVS value, portal response or log entry. It is unaffected by administrator password replacement. A privileged sign-in enables calibration only for the current local session; logout, portal expiry and restart remove that access.

```powershell
python tools/commission.py service --port <identified-port> --record provisioning/private/<unit>/service.json --operation recover_admin
```

The service tool prompts for a replacement password. The firmware accepts at most three service requests in the first 60 seconds after boot. Recovery updates only the administrator verifier. It neither formats storage nor clears device configuration. Service material is never accepted through MQTT or the web portal.
