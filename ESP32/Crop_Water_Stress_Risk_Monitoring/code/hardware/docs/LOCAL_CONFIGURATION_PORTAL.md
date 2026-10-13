# Local configuration portal

Every commissioned restart starts WPA2 AP+STA operation and a 300-second monotonic setup window. The SSID includes the last six characters of the stable device identity. At expiry, the firmware stops servicing HTTP, closes clients, invalidates local sessions/authentication jobs, disables the AP and retains STA mode. Browser refreshes do not change the deadline. An uncommissioned build leaves the AP closed.

The server accepts only clients on the AP interface. Host is restricted to `192.168.4.1` (optional port 80); POST requires origin `http://192.168.4.1` and `application/json`. HTTP headers are limited to 2,048 bytes, bodies to 12,288 bytes, and a connection to seven seconds. Duplicate content lengths and transfer encoding are rejected. Socket writes are non-blocking and limited to 512 bytes per loop.

| Endpoint | Method | Contract |
|---|---|---|
| `/api/session` | GET | Remaining seconds; session state and CSRF token when authenticated |
| `/api/login` | POST | `password`; starts a password-verification job |
| `/api/auth_result` | POST | Secret `job_id`; polls the job and sets/clears the session cookie on completion |
| `/api/logout` | POST | Invalidates the session and pending auth job |
| `/api/password` | POST | `current`, `next`, `confirm`; authenticated password-change job |
| `/api/developer/calibration` | GET / POST | Privileged-session calibration data and validated calibration writes |
| `/api/config` | GET | Sanitized saved settings |
| `/api/status` | GET | Actual connection, storage, revision and trial state |
| `/api/scan` | POST / GET | Start asynchronous scan / retrieve up to 20 results |
| `/api/trial` | POST | `kind` (`wifi` or `mqtt`), `settings`, `save`, `clear_password`, `expected_revision` |
| `/api/cancel` | POST | Cancel active connection trial and restore saved settings |
| `/api/diagnostics` | GET | Observed association, TLS/MQTT and ingestion states |

Except login/session/auth-result, routes require a session. Mutations additionally require `X-CSRF-Token`. Initial-password administrator sessions can only replace the password or sign out. The calibration route requires a session created by the privileged fixed password at the normal sign-in screen. Auth-result uses its own unpredictable single-use job identifier and same-origin POST checks.

The UI uses semantic forms, local assets and no remote dependencies. A server-derived countdown is refreshed every three seconds. At 320 px and larger tested widths, forms do not scroll horizontally. Saved passwords are never returned. Browser-side feedback distinguishes test-only restoration, saved settings and failed rollback.

HTTP errors: 400 invalid input, 401 authentication failure, 403 origin/host/first-login policy, 404 unknown endpoint, 409 revision/trial conflict, 413 request limits, 429 throttled login. The AP radio may move to the router's channel during a trial; reconnecting the browser does not extend setup time.
